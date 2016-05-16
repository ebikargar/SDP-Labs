/*
Lab 09 exercise 02

This program performs a sorting of multiple files, that contain some integer
values stored in binary representation.
The sorting is done by using a thread for each input file, that locally sorts
the values and provides the array to the main thread, that merges the results
and writes them to the output file (last parameter).
Differently from exercise 1, this time the main thread merges two results sets
as soon as they are available.
Both input files and output files contain as the first value, the number of
values contained, followed by them.

@Author: Martino Mensio
*/

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <tchar.h>
#include <assert.h>

typedef struct _DATA_STRUCT_T {
	LPTCH filename; // input
	UINT length; // output
	LPUINT vet; // output
} DATA_STRUCT_T;

typedef DATA_STRUCT_T* LPDATA_STRUCT_T;

DWORD WINAPI sortFile(LPVOID param);
VOID removeHandleAndUpdateRealIndexes(LPHANDLE handles, LPDWORD realIndexes, LPDWORD pnThreads, DWORD indexToRemove);
VOID merge(LPUINT a, UINT a_len, LPUINT b, UINT b_len, LPUINT res);

INT _tmain(INT argc, LPTSTR argv[]) {
	LPHANDLE handles;
	LPDATA_STRUCT_T data;
	UINT i, j;
	DWORD nThreads;
	LPUINT arr, arr2;
	UINT totSize, totSize2;
	HANDLE hOut;
	DWORD nOut;
	DWORD whoEnded_a, whoEnded_b;
	LPDWORD realIndexes;

	if (argc < 3) {
		_ftprintf(stderr, _T("Usage: %s list_of_input_files output_file\n"), argv[0]);
		return 1;
	}

	// first parameter is the executable, the last one is the output file
	nThreads = argc - 2;
	// allocate the array of handles for the threads
	handles = (LPHANDLE)malloc((nThreads) * sizeof(HANDLE));
	// allocate the array of real indexes where to find the data of a given thread
	realIndexes = (LPDWORD)malloc((nThreads) * sizeof(DWORD));
	// and allocate the array of structures for data sharing with the threads
	data = (LPDATA_STRUCT_T)malloc((nThreads) * sizeof(DATA_STRUCT_T));

	//check allocation
	if (handles == NULL || realIndexes == NULL || data == NULL) {
		_ftprintf(stderr, _T("Impossible to allocate some data\n"));
		return 1;
	}

	// create the threads
	for (i = 0; i < nThreads; i++) {
		// i is the index of the thread, while j is the corresponding index in the program arguments
		j = i + 1;
		// copy into the structure the pointer to the file name
		data[i].filename = argv[j];
		// and initialize the realIndex (currently the data is in the right position
		realIndexes[i] = i;
		// and launch the thread
		handles[i] = CreateThread(0, 0, sortFile, (LPVOID)&data[i], 0, NULL);
		// check the handle value
		if (handles[i] == INVALID_HANDLE_VALUE) {
			// if unable to create a thread, better to terminate immediately the process
			_ftprintf(stderr, _T("Impossible to create thread %d\n"), j);
			// the return statement on the main will exit the current process (every thread)
			return 2;
		}
	}

	// wait for the first thread to end
	whoEnded_a = WaitForMultipleObjects(nThreads, handles, FALSE, INFINITE);
	whoEnded_a -= WAIT_OBJECT_0;
	// read how may values
	totSize = data[realIndexes[whoEnded_a]].length;
	// allocate an array
	arr = (LPUINT)malloc(totSize * sizeof(UINT));
	// and copy to it the content of the ended thread
	memcpy(arr, data[realIndexes[whoEnded_a]].vet, totSize * sizeof(UINT));
	// free the array of the results of the thread (only if it has been allocated)
	if (data[realIndexes[whoEnded_a]].length > 0) {
		free(data[realIndexes[whoEnded_a]].vet);
	}
	// remove the handle from the array and play with indexes to redirect them
	removeHandleAndUpdateRealIndexes(handles, realIndexes, &nThreads, whoEnded_a);

	// while there are still some threads, wait for them
	while (nThreads > 0) {
		// wait for a thread
		whoEnded_b = WaitForMultipleObjects(nThreads, handles, FALSE, INFINITE);
		whoEnded_b -= WAIT_OBJECT_0;
		// sum up the size of the actual array and the results of the ended thread
		totSize2 = totSize + data[realIndexes[whoEnded_b]].length;
		// allocate a new array
		arr2 = (LPUINT)malloc(totSize2 * sizeof(UINT));
		// merge the actual array and results of the thread in the new array
		merge(arr, totSize, data[realIndexes[whoEnded_b]].vet, data[realIndexes[whoEnded_b]].length, arr2);
		// free the old array
		free(arr);
		// update the pointer: now arr refers to the new array
		arr = arr2;
		// update the total size
		totSize = totSize2;
		// free the results array of the ended thread
		if (data[realIndexes[whoEnded_b]].length > 0) {
			free(data[realIndexes[whoEnded_b]].vet);
		}
		// remove the handle from the array and play with indexes to redirect them
		removeHandleAndUpdateRealIndexes(handles, realIndexes, &nThreads, whoEnded_b);
	}
	// release some resources
	free(handles);
	free(realIndexes);
	free(data);
	// print the resulting array
	_tprintf(_T("Sorted array: \n"));
	for (i = 0; i < totSize; i++) {
		_tprintf(_T("%d "), arr[i]);
	}
	_tprintf(_T("\n"));

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
		return 3;
	}
	// write the integers
	if (!WriteFile(hOut, arr, totSize * sizeof(UINT), &nOut, NULL) || nOut != totSize * sizeof(UINT)) {
		_ftprintf(stderr, _T("Error writing the integers. Error: %x\n"), GetLastError());
		free(arr);
		CloseHandle(hOut);
		return 4;
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
			free(vet);
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
	
	// set the results inside the structure
	data->length = n;
	data->vet = vet;
	// release resources
	CloseHandle(hIn);
	// then return 0
	_tprintf(_T("Thread on file %s done\n"), data->filename);
	return 0;
}

// this function removes the specified handle specified as index, and updates the realIndexes array that contains
// the original positions of the structures used by threads inside the data array. They can't be moved because the threads
// still need to modify them.
// each thread uses his data passed through a pointer to the element in the array data, and the main uses data[realIndexes[i]]
// to access the data of the i-th element of the compacted array (corresponding to the realIndexes[i] element in the original array
VOID removeHandleAndUpdateRealIndexes(LPHANDLE handles, LPDWORD realIndexes, LPDWORD pnThreads, DWORD indexToRemove) {
	// can close the handle to this thread
	CloseHandle(handles[indexToRemove]);
	if (*pnThreads > 0) {
		// decrease the number of threads
		(*pnThreads)--;
		// if the ended thread is not the last one, take the last element and put in the position of the ended one
		if (indexToRemove != *pnThreads) {
			// overwrite the handle
			handles[indexToRemove] = handles[*pnThreads];
			// and say: the real position (index) of the data of this thread will be where this thread was in the original array
			realIndexes[indexToRemove] = realIndexes[*pnThreads];
		}
	}
}

VOID merge(LPUINT a, UINT a_len, LPUINT b, UINT b_len, LPUINT res) {
	DWORD i, j, k;
	for (i = 0, j = 0, k = 0; i < a_len + b_len; i++) {
		if (j == a_len) {
			// the a array ended
			assert(k < b_len);
			res[i] = b[k];
			k++;
		} else if (k == b_len) {
			// the b array ended
			assert(j < a_len);
			res[i] = a[j];
			j++;
		} else if (a[j] < b[k]) {
			assert(j < a_len);
			res[i] = a[j];
			j++;
		} else {
			assert(k < b_len);
			res[i] = b[k];
			k++;
		}
	}
}