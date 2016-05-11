/*
Lab 09 exercise 01

This program performs a sorting of multiple files, that contain some integer
values stored in binary representation.
The sorting is done by using a thread for each input file, that locally sorts
the values and provides the array to the main thread, that merges the results
and writes them to the output file (last parameter).
Both input files and output files contain as the first value, the number of
values contained, followed by them.

@Author: Martino Mensio
*/

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE

#include <Windows.h>
#include <tchar.h>

typedef struct _DATA_STRUCT_T {
	LPTCH filename; // input
	UINT length; // output
	LPUINT vet; // output
} DATA_STRUCT_T;

typedef DATA_STRUCT_T* LPDATA_STRUCT_T;

DWORD WINAPI sortFile(LPVOID param);

INT _tmain(INT argc, LPTSTR argv[]) {
	LPHANDLE handles;
	LPDATA_STRUCT_T data;
	UINT i, j, k;
	UINT nThreads;
	LPUINT arr, indexes;
	UINT totSize;
	LPBOOL endedArray;
	INT minTh;
	HANDLE hOut;
	DWORD nOut;

	if (argc < 3) {
		_ftprintf(stderr, _T("Usage: %s list_of_input_files output_file\n"), argv[0]);
		return 1;
	}

	// first parameter is the executable, the last one is the output file
	nThreads = argc - 2;
	// allocate the array of handles for the threads
	handles = (LPHANDLE)malloc((nThreads) * sizeof(HANDLE));
	// and allocate the array of structures for data sharing with the threads
	data = (LPDATA_STRUCT_T)malloc((nThreads) * sizeof(DATA_STRUCT_T));

	// create the threads
	for (i = 0; i < nThreads; i++) {
		// i is the index of the thread, while j is the corresponding index in the program arguments
		j = i + 1;
		// copy into the structure the pointer to the file name
		data[i].filename = argv[j];
		// and launch the thread
		handles[i] = CreateThread(0, 0, sortFile, (LPVOID)&data[i], 0, NULL);
		// check the handle value
		if (handles[i] == INVALID_HANDLE_VALUE) {
			// if unable to create a thread, better to terminate immediately the process
			_ftprintf(stderr, _T("Impossible to create thread %d\n"), j);
			// the return statement on the main will exit the current process (every thread)
			return 1;
		}
	}
	// wait all of them (and check return value)
	WaitForMultipleObjects(nThreads, handles, TRUE, INFINITE);
	// release resources
	for (i = 0; i < nThreads; i++) {
		CloseHandle(handles[i]);
	}
	free(handles);

	// preliminary phase to merge the results of the threads
	totSize = 0;
	// allocate an array that will contain for each thread the current index used in the merge
	indexes = (LPUINT)malloc(nThreads * sizeof(UINT));
	// this array will contain for each thread information related to the fact that we already took all the elements from his results
	endedArray = (LPBOOL)malloc(nThreads * sizeof(BOOL));
	// initialize these support variables
	for (i = 0; i < nThreads; i++) {
		// totSize contains the total number of elements
		totSize += data[i].length;
		// the starting index for the thread is 0 (no values have been read yed)
		indexes[i] = 0;
		// check on the number of elements (if a thread had some issues reading the file, the lenght was set to 0)
		if (data[i].length != 0) {
			// array with some data, can read from it
			endedArray[i] = FALSE;
		} else {
			// array is empty, this flag is set to FALSE not to read from the array
			endedArray[i] = TRUE;
		}
	}

	// allocate the array for the results of the merge
	arr = (LPUINT)malloc(totSize * sizeof(UINT));
	for (i = 0; i < totSize; i++) {
		// minTh contains the index of the thread that currently has the minimum value (candidate for being stored into the results)
		minTh = -1;
		// scan all the results of the threads
		for (j = 0; j < nThreads; j++) {
			// if i can read from this result set
			if (!endedArray[j]) {
				// i check if in the current position it contains a lower value with respect to the memorized thread
				if (minTh == -1 || data[j].vet[indexes[j]] < data[minTh].vet[indexes[minTh]]) {
					// if this is true, or if no thread has been considered yet, i memorize the index of this thread
					minTh = j;
				}
			}
		}
		// i move into the results the value (that is the current minimum among the values that i can still use
		arr[i] = data[minTh].vet[indexes[minTh]];
		// update the index of the thread that provided the value
		indexes[minTh]++;
		// and check if it can still provide values
		if (!(indexes[minTh] < data[minTh].length)) {
			// if i reached the end of its result set, i turn the flag to TRUE so that in next iterations i won't use it anymore
			endedArray[minTh] = TRUE;
		}
	}
	_tprintf(_T("Sorted array: \n"));
	for (i = 0; i < totSize; i++) {
		_tprintf(_T("%d "), arr[i]);
	}
	// release resources no more needed
	free(data);
	free(indexes);
	free(endedArray);

	// now comes the output file
	hOut = CreateFile(argv[argc - 1], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	// check the handle
	if (hOut == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Unable to create file %s. Error: %x\n"), argv[argc - 1], GetLastError());
	}
	// write the number of integers
	if (!WriteFile(hOut, &totSize, sizeof(totSize), &nOut, NULL) || nOut != sizeof(totSize)) {
		_ftprintf(stderr, _T("Error writing the total nunmber of integers. Error: %x\n"), GetLastError());
		free(arr);
		CloseHandle(hOut);
		return 2;
	}
	// write the integers
	if (!WriteFile(hOut, arr, totSize * sizeof(UINT), &nOut, NULL) || nOut != totSize * sizeof(UINT)) {
		_ftprintf(stderr, _T("Error writing the integers. Error: %x\n"), GetLastError());
		free(arr);
		CloseHandle(hOut);
		return 3;
	}

	_tprintf(_T("Written correctly to output file %s\n"), argv[argc - 1]);
	free(arr);
	CloseHandle(hOut);

	return 0;
}

// the output of this function is stored inside the structure passed as a parameter
DWORD WINAPI sortFile(LPVOID param) {
	LPDATA_STRUCT_T data = (LPDATA_STRUCT_T)param;
	HANDLE hIn;
	UINT n;
	DWORD nRead;
	LPUINT vet;

	// open the binary file for reading
	hIn = CreateFile(data->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	// check the HANDLE value
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open input file %s. Error: %x\n"), data->filename, GetLastError());
		// set the length to 0 so the main won't read the array
		data->length = 0;
		return 0;
	}
	// read dimension (is the first integer inside the file)
	if (!ReadFile(hIn, &n, sizeof(n), &nRead, NULL) || nRead != sizeof(n)) {
		_ftprintf(stderr, _T("Error reading the number of integers in file %s. Error: %x\n"), data->filename, GetLastError());
		data->length = 0;
		// set the length to 0 so the main won't read the array
		return 0;
	}

	// allocate array of proper size
	vet = (LPUINT)malloc(n * sizeof(UINT));

	// then i read all the values
	for (UINT i = 0; i < n; i++) {
		if (!ReadFile(hIn, &vet[i], sizeof(n), &nRead, NULL) || nRead != sizeof(n)) {
			_ftprintf(stderr, _T("Error reading the integers in file %s. In this file there should be %u of them. Error: %x\n"), data->filename, n, GetLastError());
			data->length = 0;
			return 0;
		}
	}

	// sort array (bubble sort)
	UINT l = 0;
	UINT r = n - 1;
	UINT i, j, temp;
	for (i = l; i < r; i++) {
		for (j = l; j < r - i + l; j++) {
			if (vet[j] > vet[j + 1]) {
				temp = vet[j];
				vet[j] = vet[j + 1];
				vet[j + 1] = temp;
			}
		}
	}
	/*
	for (i = 0; i < n; i++) {
		_tprintf(_T("%d "), vet[i]);
	}
	*/
	// set the results inside the structure
	data->length = n;
	data->vet = vet;
	// release resources
	CloseHandle(hIn);
	// then return 0
	return 0;
}
