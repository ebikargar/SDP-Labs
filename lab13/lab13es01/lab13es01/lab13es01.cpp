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
#define MAX_OUTPUT_FILES 128
#define SWAP_TIMEOUT 50		// time to change part of WaitForMultipleObjects with more than 64 objects
#define TERMINATION_SEMAPHORE_NAME _T("TerminationSemaphore")
#define MAX_SEM_VALUE 10000	// upper bound for semaphore

typedef struct _record {
	TCHAR directoryName[NAME_MAX_LEN];
	TCHAR inputFileName[NAME_MAX_LEN];
	TCHAR outputName[NAME_MAX_LEN];
} RECORD, *LPRECORD;

typedef struct _outputRecord {
	DWORD32 nFiles;
	DWORD32 nChar;
	DWORD32 nNewLines;
} OUTPUTRECORD, *LPOUTPUTRECORD;

typedef struct _processingParam {
	HANDLE hRecordsFile;
	CRITICAL_SECTION cs;
	DWORD nProcessing;
} PROCESSINGPARAM, *LPPROCESSINGPARAM;

typedef struct _collectorParam {
	LPTSTR filename;
	DWORD updateFileInterval;
	DWORD nProcessing;	// collector needs to know how many processing threads
} COLLECTORPARAM, *LPCOLLECTORPARAM;

BOOL rotateBackslashes(LPTSTR stringToManipulate);
DWORD WINAPI processingThreadFunction(LPVOID);
DWORD WINAPI collectorThreadFunction(LPVOID);

BOOL outputRecordRead(HANDLE hFile, LPOUTPUTRECORD outputRecord);
BOOL outputRecordWrite(HANDLE hFile, LPOUTPUTRECORD outputRecord);

DWORD WaitForVeryHugeNumberOfObjects(LPHANDLE handles, DWORD count, BOOL waitForAll);

INT _tmain(INT argc, LPTSTR argv[]) {
	HANDLE hIn;
	INT N, M;
	INT i, j;
	LPHANDLE hThreads;
	COLLECTORPARAM cp;
	PROCESSINGPARAM pp;

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
	pp.nProcessing = N;

	// create threads
	for (i = 0; i < N; i++) {
		hThreads[i] = CreateThread(NULL, 0, processingThreadFunction, &pp, 0, NULL);
		if (hThreads[i] == NULL) {
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
	cp.nProcessing = N;

	// create last thread
	hThreads[N] = CreateThread(NULL, 0, collectorThreadFunction, &cp, 0, NULL);
	if (hThreads[N] == NULL) {
		_ftprintf(stderr, _T("Impossible to create last thread. Error: %x"), GetLastError());
		for (i = 0; i < N; i++) {
			TerminateThread(hThreads[i], 1);
			CloseHandle(hThreads[i]);
		}
		free(hThreads);
		return 7;
	}
	
	// wait all the threads
	if (WaitForVeryHugeNumberOfObjects(hThreads, N, TRUE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Impossible to wait for all the threads\n"));
		for (i = 0; i < N + 1; i++) {
			TerminateThread(hThreads[i], 1);
			CloseHandle(hThreads[i]);
		}
		free(hThreads);
		return 8;
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
	HANDLE terminationSemaphore;
	OUTPUTRECORD outputRecord;

	//_tprintf(_T("I am alive\n"));
	terminationSemaphore = CreateSemaphore(NULL, 0, pp->nProcessing, TERMINATION_SEMAPHORE_NAME);
	if (terminationSemaphore == NULL) {
		_ftprintf(stderr, _T("Impossible to open the termination semaphore. Error: %x\n"), GetLastError());
		return 1;
	}
	
	while (TRUE) {
		EnterCriticalSection(&pp->cs);
		result = ReadFile(pp->hRecordsFile, &record, sizeof(RECORD), &nRead, NULL);
		LeaveCriticalSection(&pp->cs);
		if (result == FALSE) {
			_ftprintf(stderr, _T("Impossible to read a record from the input file. Error: %x\n"), GetLastError());
			return 1;
		}
		if (nRead == 0) {
			// end of file
			Sleep(1000);
			// TODO signal on termination semaphore
			if (!ReleaseSemaphore(terminationSemaphore, 1, NULL)) {
				_ftprintf(stderr, _T("Impossible to release termination semaphore. Error: %x\n"), GetLastError());
				return 2;
			}
			//protectedBoolSet(pp->pb, TRUE);
			break;
		}
		if (nRead != sizeof(RECORD)) {
			_ftprintf(stderr, _T("Record size mismatch\n"));
			return 3;
		}
		_tprintf(_T("Record read: %s %s %s\n"), record.directoryName, record.inputFileName, record.outputName);
		
		//Sleep(1000);
		// TODO: do some job (scan directory and count things)
		/*
		scan directory
		for each file, process it (counting chars and newlines) and add to the outputRecord (may need realloc)
		*/
		outputRecord.nFiles = 3;
		outputRecord.nChar = 127;
		outputRecord.nNewLines = 28;


		_tcsncpy(semaphoreOutputName, PREFIX_NAME_SEM, NAME_MAX_LEN);
		_tcsncat(semaphoreOutputName, record.outputName, NAME_MAX_LEN - PREFIX_NAME_LEN);
		rotateBackslashes(semaphoreOutputName);
		_tcsncpy(mutexOutputName, PREFIX_NAME_MUT, NAME_MAX_LEN);
		_tcsncat(mutexOutputName, record.outputName, NAME_MAX_LEN - PREFIX_NAME_LEN);
		rotateBackslashes(mutexOutputName);

		// Named Mutex: CreateMutex returns the existing mutex if it has already been created
		hMutexOutput = CreateMutex(NULL, FALSE, mutexOutputName);
		if (hMutexOutput == NULL) {
			_ftprintf(stderr, _T("Impossible to create/open mutex. Error: %x\n"), GetLastError());
			return 4;
		}
		// Named Semaphore: CreateSemaphore returns the existing semaphore if it has already been created
		hSemaphoreOutput = CreateSemaphore(NULL, 0, MAX_SEM_VALUE, semaphoreOutputName);
		if (hSemaphoreOutput == NULL) {
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
		//WriteFile(hFileOutput, _T("ciao"), 5 * sizeof(TCHAR), &nWritten, NULL);
		if (!outputRecordWrite(hFileOutput, &outputRecord)) {
			_ftprintf(stderr, _T("Impossible to write output record. Error: %x\n"), GetLastError());
			return 9;
		}
		CloseHandle(hFileOutput);
		// release the mutex for the file
		ReleaseMutex(hMutexOutput);

		// Signal that a record has been written on the output file
		if (!ReleaseSemaphore(hSemaphoreOutput, 1, NULL)) {
			_ftprintf(stderr, _T("Impossible to release the semaphore for output file %s. Error: %x\n"), record.outputName, GetLastError());
			return 10;
		}
	}

	return 0;
}

DWORD WINAPI collectorThreadFunction(LPVOID p) {
	LPCOLLECTORPARAM cp = (LPCOLLECTORPARAM)p;
	HANDLE hRecordsFile;
	//HANDLE hCurrentOutputFile;
	RECORD record;
	DWORD nRead, nOutputFiles, i;
	LONG found;
	TCHAR outputFileNames[MAX_OUTPUT_FILES][NAME_MAX_LEN];
	TCHAR semaphoreOutputName[NAME_MAX_LEN + PREFIX_NAME_LEN];
	HANDLE hSemaphoresOutputAndTerminationSemaphore[MAX_OUTPUT_FILES + 1];
	DWORD nTerminated;
	DWORD rowsReady[MAX_OUTPUT_FILES];
	HANDLE hOutputFiles[MAX_OUTPUT_FILES];
	DWORD result;
	OUTPUTRECORD outRecord;
	memset(outputFileNames, 0, sizeof(outputFileNames));

	hRecordsFile = CreateFile(cp->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hRecordsFile == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to open the records file. Error: %x\n"), GetLastError());
		return 2;
	}
	// read the file to get the output file names
	nOutputFiles = 0;
	while (TRUE) {
		if (!ReadFile(hRecordsFile, &record, sizeof(RECORD), &nRead, NULL)) {
			_ftprintf(stderr, _T("Impossible to read from records file. Error: %x\n"), GetLastError());
			CloseHandle(hRecordsFile);
			return 3;
		}
		if (nRead == 0) {
			break;
		}
		found = -1;
		for (i = 0; i < nOutputFiles; i++) {
			if (_tcsncmp(outputFileNames[i], record.outputName, NAME_MAX_LEN) == 0) {
				found = i;
				break;
			}
		}
		if (found == -1) {
			if (nOutputFiles >= MAX_OUTPUT_FILES) {
				_ftprintf(stderr, _T("Too many records in this file\n"));
				CloseHandle(hRecordsFile);
				return 4;
			}
			_tcsncpy(outputFileNames[i], record.outputName, NAME_MAX_LEN);
			nOutputFiles++;
		}
	}
	CloseHandle(hRecordsFile);

	// Now open all the semaphores for the output files
	
	for (i = 0; i < nOutputFiles; i++) {
		_tcsncpy(semaphoreOutputName, PREFIX_NAME_SEM, NAME_MAX_LEN);
		_tcsncat(semaphoreOutputName, outputFileNames[i], NAME_MAX_LEN - PREFIX_NAME_LEN);
		rotateBackslashes(semaphoreOutputName);

		hSemaphoresOutputAndTerminationSemaphore[i] = CreateSemaphore(NULL, 0, MAX_SEM_VALUE, semaphoreOutputName);
		if (hSemaphoresOutputAndTerminationSemaphore[i] == NULL) {
			_ftprintf(stderr, _T("Impossible to open semaphore for output file (collector). Error: %x\n"), GetLastError());
			return 1;
		}
		rowsReady[i] = 0;
		hOutputFiles[i] = CreateFile(outputFileNames[i], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOutputFiles[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to open output file %s. Error: %x"), outputFileNames[i], GetLastError());
			return 2;
		}
	}
	// the last handle of the array is the termination Semaphore (signalled by threads that reach end of records file
	hSemaphoresOutputAndTerminationSemaphore[nOutputFiles] = CreateSemaphore(NULL, 0, cp->nProcessing, TERMINATION_SEMAPHORE_NAME);
	if (hSemaphoresOutputAndTerminationSemaphore[nOutputFiles] == NULL) {
		_ftprintf(stderr, _T("Impossible to open termination semaphore (collector). Error: %x\n"), GetLastError());
		return 1;
	}
	nTerminated = 0;

	while (nTerminated < cp->nProcessing) {
		result = WaitForVeryHugeNumberOfObjects(hSemaphoresOutputAndTerminationSemaphore, nOutputFiles + 1, FALSE);
		if (result == WAIT_FAILED) {
			_ftprintf(stderr, _T("Impossible to wait very huge number of objects (collector)"));
			return 1;
		}
		assert(result <= nOutputFiles);
		if (result == nOutputFiles) {
			// a thread reached end of input file
			nTerminated++;
		} else {
			// a thread wrote on some output file
			rowsReady[result]++;
			if (rowsReady[result] == cp->updateFileInterval) {
				_tprintf(_T("File %s:\n"), outputFileNames[result]);
				// time to process this file
				for (i = 0; i < cp->updateFileInterval; i++) {
					if (!outputRecordRead(hOutputFiles[result], &outRecord)) {
						_ftprintf(stderr, _T("Impossible to read output record\n"));
						return 2;
					}
					_tprintf(_T("\tn_files: %u\tn_char: %u\tn_newlines: %u\n"), outRecord.nFiles, outRecord.nChar, outRecord.nNewLines);
				}
			}
		}
	}
	// TODO understand why need purging, although Sleep before terminate semaphore
	_tprintf(_T("Purging output files\n"));
	for (i = 0; i < nOutputFiles; i++) {
		_tprintf(_T("File %s:\n"), outputFileNames[i]);
		while (outputRecordRead(hOutputFiles[i], &outRecord)) {
			_tprintf(_T("\tn_files: %u\tn_char: %u\tn_newlines: %u\n"), outRecord.nFiles, outRecord.nChar, outRecord.nNewLines);
		}
		_tprintf(_T("OK\n"));
	}
	return 0;
}

BOOL outputRecordRead(HANDLE hFile, LPOUTPUTRECORD outputRecord) {
	DWORD nRead;
	if (!ReadFile(hFile, outputRecord, sizeof(OUTPUTRECORD), &nRead, NULL)) {
		_ftprintf(stderr, _T("Impossible to read nFiles inside an output record. Error: %x\n"), GetLastError());
		return FALSE;
	}
	if (nRead == 0) {
		return FALSE;
	}
	if (nRead != sizeof(OUTPUTRECORD)) {
		_ftprintf(stderr, _T("Read size mismatch on nFiles inside an output record\n"));
		return FALSE;
	}
	return TRUE;
}
BOOL outputRecordWrite(HANDLE hFile, LPOUTPUTRECORD outputRecord) {
	DWORD nWritten;
	if (!WriteFile(hFile, outputRecord, sizeof(OUTPUTRECORD), &nWritten, NULL)) {
		_ftprintf(stderr, _T("Impossible to write nFiles inside an output record. Error: %x\n"), GetLastError());
		return FALSE;
	}
	if (nWritten != sizeof(OUTPUTRECORD)) {
		_ftprintf(stderr, _T("Write size mismatch on nFiles inside an output record\n"));
		return FALSE;
	}
	return TRUE;
}

// if WaitForAll == TRUE returns 0 or WAIT_FAILED
// if WaitForAll == FALSE returns the index of the handle ready
// if WaitForAll == FALSE and count greater than 64, the wait is done with timeout on groups of 64 members
DWORD WaitForVeryHugeNumberOfObjects(LPHANDLE handles, DWORD count, BOOL waitForAll) {
	DWORD waited, toWait, toWaitNow, result, nChunks, i, chunkN;
	toWait = count;
	waited = 0;
	if (waitForAll) {
		// wait for all the handles
		while (toWait > 0) {
			toWaitNow = (toWait > MAXIMUM_WAIT_OBJECTS) ? MAXIMUM_WAIT_OBJECTS : toWait;
			if (WaitForMultipleObjects(toWaitNow, &handles[waited], TRUE, INFINITE) == WAIT_FAILED) {
				return WAIT_FAILED;
			}
			waited += toWaitNow;
			toWait -= toWaitNow;
		}
		return 0;
	} else {
		// just want to wait for one of them
		if (count > MAXIMUM_WAIT_OBJECTS) {
			nChunks = count / MAXIMUM_WAIT_OBJECTS;
			// wait on chuncks with timeout
			chunkN = 0;
			while (TRUE) {
				// compute on how many i will wait
				toWaitNow = (chunkN == nChunks - 1) ? (count - MAXIMUM_WAIT_OBJECTS * chunkN) : MAXIMUM_WAIT_OBJECTS;
				result = WaitForMultipleObjects(toWaitNow, &handles[chunkN * MAXIMUM_WAIT_OBJECTS], FALSE, SWAP_TIMEOUT);
				if (result == WAIT_TIMEOUT) {
					// i want to wait for the next chunk
					chunkN = (chunkN + 1) % nChunks;
					continue;
				} else if (result == WAIT_FAILED) {
					_ftprintf(stderr, _T("WaitForMultipleObjects WAIT_FAILED. Error: %x"), GetLastError());
					return WAIT_FAILED;
				} else {
					result -= WAIT_OBJECT_0;
					// compute the index of the original array of handles
					result += chunkN * MAXIMUM_WAIT_OBJECTS;
					return result;
				}

				
			}
		} else {
			// less than WAIT_OBJECT_0 handles
			return WaitForMultipleObjects(count, handles, FALSE, INFINITE) - WAIT_OBJECT_0;
		}
	}
	
}