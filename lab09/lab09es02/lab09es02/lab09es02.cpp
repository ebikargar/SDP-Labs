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
VOID removeHandleAndData(LPHANDLE handles, LPDATA_STRUCT_T data, LPDWORD nThreads, DWORD whoEnded_a);
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
	
	// TODO checks on number of threads (assumed that at least 2 threads provide results)
	whoEnded_a = WaitForMultipleObjects(nThreads, handles, FALSE, INFINITE);
	whoEnded_a -= WAIT_OBJECT_0;
	totSize = data[whoEnded_a].length;
	// TODO check totSize (find the first thread with some content to provide)
	arr = (LPUINT)malloc(totSize * sizeof(UINT));
	memcpy(arr, data[whoEnded_a].vet, totSize * sizeof(UINT));
	removeHandleAndData(handles, data, &nThreads, whoEnded_a);

	while (nThreads > 0) {
		whoEnded_b = WaitForMultipleObjects(nThreads, handles, FALSE, INFINITE);
		whoEnded_b -= WAIT_OBJECT_0;
		totSize2 = totSize + data[whoEnded_b].length;
		// TODO also there chech if thread can provide data
		arr2 = (LPUINT)malloc(totSize2 * sizeof(UINT));
		merge(arr, totSize, data[whoEnded_b].vet, data[whoEnded_b].length, arr2);
		free(arr);
		arr = arr2;
		totSize = totSize2;
		removeHandleAndData(handles, data, &nThreads, whoEnded_b);
	}
	free(handles);

	
	_tprintf(_T("Sorted array: \n"));
	for (i = 0; i < totSize; i++) {
		_tprintf(_T("%d "), arr[i]);
	}
	_tprintf(_T("\n"));
	// release resources no more needed
	free(data);

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
	_tprintf(_T("Thread on file %s done\n"), data->filename);
	return 0;
}

VOID removeHandleAndData(LPHANDLE handles, LPDATA_STRUCT_T data, LPDWORD pnThreads, DWORD indexToRemove) {
	CloseHandle(handles[indexToRemove]);
	if (data[indexToRemove].length > 0) {
		free(data[indexToRemove].vet);
		data[indexToRemove].vet = NULL;
	}
	if (*pnThreads > 0) {
		(*pnThreads)--;
		if (indexToRemove != *pnThreads) {
			handles[indexToRemove] = handles[*pnThreads];
			data[indexToRemove] = data[*pnThreads];
		}
	}
}

VOID merge(LPUINT a, UINT a_len, LPUINT b, UINT b_len, LPUINT res) {
	DWORD i, j, k;
	for (i = 0, j = 0, k = 0; i < a_len + b_len; i++) {
		if (j == a_len) {
			assert(k < b_len);
			res[i] = b[k];
			k++;
		} else if (k == b_len) {
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