/*
Lab 13 exercise 01

TODO description

@Author: Martino Mensio
*/

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>

#define NAME_MAX_LEN 128
#define LINE_MAX_LEN 128*3 + 2 + 2 + 1 // two spaces, CR, LF, \0

#define PREFIX_NAME_LEN 3
#define PREFIX_NAME_SEM _T("SEM")
#define PREFIX_NAME_MUT _T("MUT")

typedef struct _record {
	TCHAR directoryName[NAME_MAX_LEN];
	TCHAR inputFileName[NAME_MAX_LEN];
	TCHAR outputName[NAME_MAX_LEN];
} RECORD, *LPRECORD;

typedef struct _fileInfoCollected {
	DWORD nChar;
	DWORD nNewLines;
} FILEINFOCOLLECTED, *LPFILEINFOCOLLECTED;

typedef struct _outputRecord {
	DWORD32 nFiles;
	LPFILEINFOCOLLECTED filesInfo;	// array
} OUTPUTRECORD, *LPOUTPUTRECORD;

typedef struct _processingParam {
	HANDLE hRecordsFile;
	CRITICAL_SECTION cs;
} PROCESSINGPARAM, *LPPROCESSINGPARAM;

typedef struct _collectorParam {
	LPTSTR filename;
	INT updateFileInterval;
} COLLECTORPARAM, *LPCOLLECTORPARAM;

BOOL rotateBackslashes(LPTSTR stringToManipulate);
DWORD WINAPI processingThreadFunction(LPVOID);
DWORD WINAPI collectorThreadFunction(LPVOID);

BOOL outputRecordRead(HANDLE hFile, LPOUTPUTRECORD outputRecord);
BOOL outputRecordWrite(HANDLE hFile, LPOUTPUTRECORD outputRecord);

INT _tmain(INT argc, LPTSTR argv[]) {
	HANDLE hIn;
	INT N, M;
	INT i, j;
	LPHANDLE hThreads;
	COLLECTORPARAM cp;
	PROCESSINGPARAM pp;
	INT threadsWaited, toBeWaited;

	// check number of parameters
	if (argc != 4) {
		_ftprintf(stderr, _T("Usage: %s input_file N M\n"), argv[0]);
		return 1;
	}
	N = _tstoi(argv[2]);
	if (N <= 0) {
		_ftprintf(stderr, _T("N must be a positive number\n"));
		return 2;
	}
	M = _tstoi(argv[3]);
	if (M <= 0) {
		_ftprintf(stderr, _T("M must be a positive number\n"));
		return 3;
	}
	// open file for reading
	hIn = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to open input file. Error: %x\n"), GetLastError());
		return 4;
	}

	hThreads = (LPHANDLE)calloc(N + 1, sizeof(HANDLE));
	if (hThreads == NULL) {
		_ftprintf(stderr, _T("Impossible to allocate handles for threads\n"));
		return 5;
	}

	pp.hRecordsFile = hIn;
	InitializeCriticalSection(&pp.cs);

	// create threads
	for (i = 0; i < N; i++) {
		hThreads[i] = CreateThread(NULL, 0, processingThreadFunction, &pp, 0, NULL);
		if (hThreads[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create a thread. Error: %x\n"), GetLastError());
			for (j = 0; j < i; j++) {
				// kill all the already created threads
				TerminateThread(hThreads[j], 1);
				CloseHandle(hThreads[i]);
			}
			free(hThreads);
			return 6;
		}
	}

	cp.filename = argv[1];
	cp.updateFileInterval = M;

	// create last thread
	hThreads[N] = CreateThread(NULL, 0, collectorThreadFunction, &cp, 0, NULL);
	if (hThreads[N] == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create last thread. Error: %x"), GetLastError());
		for (i = 0; i < N; i++) {
			TerminateThread(hThreads[i], 1);
			CloseHandle(hThreads[i]);
		}
		free(hThreads);
		return 7;
	}

	// considering MAXIMUM_WAIT_OBJECTS
	threadsWaited = 0;
	while (threadsWaited < N) {
		toBeWaited = ((N - threadsWaited) > MAXIMUM_WAIT_OBJECTS) ? MAXIMUM_WAIT_OBJECTS : (N - threadsWaited);
		if (WaitForMultipleObjects(toBeWaited, &hThreads[threadsWaited], TRUE, INFINITE) == WAIT_FAILED) {
			_ftprintf(stderr, _T("Impossible to wait for all the threads\n"));
			for (i = 0; i < N + 1; i++) {
				TerminateThread(hThreads[i], 1);
				CloseHandle(hThreads[i]);
			}
			free(hThreads);
			return 8;
		}
		threadsWaited += toBeWaited;
	}
	
	
	for (i = 0; i < N; i++) {
		CloseHandle(hThreads[i]);
	}

	// TODO: signal to the collector that the work has finished

	if (WaitForSingleObject(hThreads[N], INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Impossible to wait for the last thread. Error: %x\n"), GetLastError());
		TerminateThread(hThreads[N], 1);
		CloseHandle(hThreads[N]);
		return 9;
	}

	_tprintf(_T("Program going to terminate correctly\n"));
	CloseHandle(hThreads[N]);

	free(hThreads);

	return 0;
}

// named semaphores and mutexes can't have backslashes in the name
BOOL rotateBackslashes(LPTSTR stringToManipulate) {
	DWORD i;
	ULONGLONG strLen = _tcsnlen(stringToManipulate, NAME_MAX_LEN);
	if (strLen >= NAME_MAX_LEN) {
		return FALSE;
	}
	for (i = 0; i < strLen; i++) {
		if (stringToManipulate[i] == _T('\\')) {
			stringToManipulate[i] = _T('/');
		}
	}
	return TRUE;
}

DWORD WINAPI processingThreadFunction(LPVOID p) {
	LPPROCESSINGPARAM pp = (LPPROCESSINGPARAM)p;
	DWORD nRead, nWritten;
	RECORD record;
	BOOL result;
	HANDLE hMutexOutput, hSemaphoreOutput, hFileOutput;
	TCHAR mutexOutputName[NAME_MAX_LEN + PREFIX_NAME_LEN];
	TCHAR semaphoreOutputName[NAME_MAX_LEN + PREFIX_NAME_LEN];
	
	while (TRUE) {
		EnterCriticalSection(&pp->cs);
		result = ReadFile(pp->hRecordsFile, &record, sizeof(RECORD), &nRead, NULL);
		LeaveCriticalSection(&pp->cs);
		if (result == FALSE) {
			_ftprintf(stderr, _T("Impossible to read a record from the input file. Error: %x\n"), GetLastError());
			return 2;
		}
		if (nRead == 0) {
			// end of file
			break;
		}
		if (nRead != sizeof(RECORD)) {
			_ftprintf(stderr, _T("Record size mismatch\n"));
			return 3;
		}
		_tprintf(_T("Record read: %s %s %s\n"), record.directoryName, record.inputFileName, record.outputName);
		
		Sleep(1000);
		// TODO: do some job (scan directory and count things)
		/*
		scan directory
		for each file, process it (counting chars and newlines) and add to the outputRecord (may need realloc)
		*/


		_tcsncpy(semaphoreOutputName, PREFIX_NAME_SEM, NAME_MAX_LEN);
		_tcsncat(semaphoreOutputName, record.outputName, NAME_MAX_LEN - PREFIX_NAME_LEN);
		rotateBackslashes(semaphoreOutputName);
		_tcsncpy(mutexOutputName, PREFIX_NAME_MUT, NAME_MAX_LEN);
		_tcsncat(mutexOutputName, record.outputName, NAME_MAX_LEN - PREFIX_NAME_LEN);
		rotateBackslashes(mutexOutputName);

		// Named Mutex: CreateMutex returns the existing mutex if it has already been created
		hMutexOutput = CreateMutex(NULL, FALSE, mutexOutputName);
		if (hMutexOutput == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create/open mutex. Error: %x\n"), GetLastError());
			return 4;
		}
		// Named Semaphore: CreateSemaphore returns the existing semaphore if it has already been created
		hSemaphoreOutput = CreateSemaphore(NULL, 0, INFINITE, semaphoreOutputName);
		if (hSemaphoreOutput == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create/open semaphore. Error: %x\n"), GetLastError());
			return 5;
		}

		// acquire the mutex for the output file
		if (WaitForSingleObject(hMutexOutput, INFINITE) == WAIT_FAILED) {
			_ftprintf(stderr, _T("Impossible to wait for the semaphore named %s. Error: %x"), mutexOutputName, GetLastError());
			return 6;
		}
		// write the data

		// open new/existing file
		hFileOutput = CreateFile(record.outputName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFileOutput == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to open/create file %s. Error: %x\n"), record.outputName, GetLastError());
			ReleaseMutex(hMutexOutput);
			return 7;
		}
		// go to the end
		if (SetFilePointer(hFileOutput, 0, 0, FILE_END) == INVALID_SET_FILE_POINTER) {
			_ftprintf(stderr, _T("Impossible to set the file pointer to the end of the file %s. Error: %x\n"), record.outputName, GetLastError());
			ReleaseMutex(hMutexOutput);
			return 8;
		}
		// write
		WriteFile(hFileOutput, _T("ciao"), 5 * sizeof(TCHAR), &nWritten, NULL);
		CloseHandle(hFileOutput);
		// release the mutex for the file
		ReleaseMutex(hMutexOutput);
	}

	return 0;
}

DWORD WINAPI collectorThreadFunction(LPVOID) {

	return 0;
}

BOOL outputRecordRead(HANDLE hFile, LPOUTPUTRECORD outputRecord) {
	DWORD nRead;
	if (!ReadFile(hFile, &outputRecord->nFiles, sizeof(DWORD32), &nRead, NULL)) {
		_ftprintf(stderr, _T("Impossible to read nFiles inside an output record. Error: %x\n"), GetLastError());
		return FALSE;
	}
	if (nRead != sizeof(DWORD32)) {
		_ftprintf(stderr, _T("Read size mismatch on nFiles inside an output record\n"));
		return FALSE;
	}
	if (!ReadFile(hFile, &outputRecord->filesInfo, outputRecord->nFiles * sizeof(FILEINFOCOLLECTED), &nRead, NULL)) {
		_ftprintf(stderr, _T("Impossible to read filesInfo inside an output record. Error: %x\n"), GetLastError());
		return FALSE;
	}
	if (nRead != outputRecord->nFiles * sizeof(DWORD32)) {
		_ftprintf(stderr, _T("Read size mismatch on filesInfo inside an output record\n"));
		return FALSE;
	}
	return TRUE;
}
BOOL outputRecordWrite(HANDLE hFile, LPOUTPUTRECORD outputRecord) {
	DWORD nWritten;
	if (!WriteFile(hFile, &outputRecord->nFiles, sizeof(DWORD32), &nWritten, NULL)) {
		_ftprintf(stderr, _T("Impossible to write nFiles inside an output record. Error: %x\n"), GetLastError());
		return FALSE;
	}
	if (nWritten != sizeof(DWORD32)) {
		_ftprintf(stderr, _T("Write size mismatch on nFiles inside an output record\n"));
		return FALSE;
	}
	if (!WriteFile(hFile, &outputRecord->filesInfo, outputRecord->nFiles * sizeof(FILEINFOCOLLECTED), &nWritten, NULL)) {
		_ftprintf(stderr, _T("Impossible to write filesInfo inside an output record. Error: %x\n"), GetLastError());
		return FALSE;
	}
	if (nWritten != outputRecord->nFiles * sizeof(DWORD32)) {
		_ftprintf(stderr, _T("Write size mismatch on filesInfo inside an output record\n"));
		return FALSE;
	}
	return TRUE;
}