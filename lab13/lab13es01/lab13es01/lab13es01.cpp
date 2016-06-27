/*
Lab 13 exercise 01

Description of exercise is provided into the file lab13/lab13.txt

Use the provided BinaryGenerator to generate the input file.

In order to obtain the expected results, delete the output files before each run of the program.
If this is not done, this program will append the content to the existing one.

NAMED SEMAPHORES AND MUTEXES:

In order to synchronize on the files between threads, i have used NAMED semaphores and mutexes.
Since each thread does not know the existence of others (only how many of them in some cases) those
synchronization objects are not passed as handles, but are globally handled by Windows libraries.
When a CreateSemaphore or CreateMutex is done with a name, an object of the corresponding type is
created only if it does not exist yet. If it already exists, those function return an handle to the
already existing one.
For each output file there is a named Semaphore and a named Mutex.
The mutex is used by the writers (who don't know each others) in order to have mutual exclusion with
respect to the writing phase.
The semaphore is used to signal to the "collector" thread that a row has been added. When the value of
the semaphore reaches M, the collector processes M records.
There is another global named Semaphore that is used to signal how many reading threads have reached
the end of the file.
The collector can use a WaitForMultipleObject (modified in order to go beyond the limit of MAXIMUM_WAIT_OBJECTS)
in order to do something like a UNIX select, and update his own counters to understand what is happening.

READING/PROCESSING THREADS:

The reading threads share the same handle in order to read the input file, and to manage concurrent access
a critical section is used to protect the reading. In this way, the internal file pointer is automatically
increased and there is no need to set it using system calls.

Instead, the writing on the output file (specified in each row) is done with a handle that is created every time
by the reading threads. They must write at the end of the file, so there are calls to the SetFilePointer.

COLLECTOR THREAD:

On the other side, the collector thread has his own handle to read the input file, and stores in an array the names
of the output files and opens it (keeping file handles to them into an array).

The output files are read with the file hadles created in the set-up phase, and there is no need to update the file
pointers because the handles used are private.

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
#define LINE_MAX_LEN 512 // for scanned files in the visit of the directories

#define PREFIX_NAME_LEN 3
#define PREFIX_NAME_SEM _T("SEM")
#define PREFIX_NAME_MUT _T("MUT")
#define MAX_OUTPUT_FILES 128
#define SWAP_TIMEOUT 50		// time to change part of WaitForMultipleObjects with more than 64 objects
#define TERMINATION_SEMAPHORE_NAME _T("TerminationSemaphore")
#define MAX_SEM_VALUE 10000	// upper bound for semaphore

#define TYPE_FILE 1
#define TYPE_DIR 2
#define TYPE_DOT 3

typedef struct _record {				// structure of a record in the input file
	TCHAR directoryName[NAME_MAX_LEN];
	TCHAR inputFileName[NAME_MAX_LEN];
	TCHAR outputName[NAME_MAX_LEN];
} RECORD, *LPRECORD;

typedef struct _outputRecord {			// structure of a record into the output files
	DWORD32 nFiles;		// number of files processed
	DWORD32 nChar;		// total number of characters in the files processed
	DWORD32 nNewLines;	// total number of newlines in the files processed
} OUTPUTRECORD, *LPOUTPUTRECORD;

typedef struct _processingParam {		// structure of a parameter passed to a processing thread
	HANDLE hRecordsFile;	// shared handle on the input file for records
	CRITICAL_SECTION cs;	// critical section to have mutual exclusion on the recordsFile
	DWORD nProcessing;		// how many processing threads are we? (used in some semaphore maxCount initialization)
} PROCESSINGPARAM, *LPPROCESSINGPARAM;

typedef struct _collectorParam {		// structure of a parameter passed to the collector thread
	LPTSTR filename;			// name of the input file for the records (collector will have his own handle for this file)
	DWORD updateFileInterval;	// M
	DWORD nProcessing;			// collector needs to know how many processing threads are there, to understand when they all have finished
} COLLECTORPARAM, *LPCOLLECTORPARAM;

BOOL rotateBackslashes(LPTSTR stringToManipulate);
DWORD WINAPI processingThreadFunction(LPVOID);
DWORD WINAPI collectorThreadFunction(LPVOID);

BOOL outputRecordRead(HANDLE hFile, LPOUTPUTRECORD outputRecord);
BOOL outputRecordWrite(HANDLE hFile, LPOUTPUTRECORD outputRecord);

DWORD WaitForVeryHugeNumberOfObjects(LPHANDLE handles, DWORD count, BOOL waitForAll);

VOID visitDirectoryAndDo(LPTSTR path1, LPTSTR search, DWORD level, VOID(*toDo)(LPTSTR path, LPOUTPUTRECORD outputRecord), LPOUTPUTRECORD outputRecord);
static DWORD FileType(LPWIN32_FIND_DATA pFileData);
VOID updateOutputRecord(LPTSTR path, LPOUTPUTRECORD outputRecord);

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
		CloseHandle(hIn);
		return 5;
	}

	// prepare the processing parameter (one for all threads)
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
			CloseHandle(hIn);
			return 6;
		}
	}

	// prepare the collector parameter
	cp.filename = argv[1];
	cp.updateFileInterval = M;
	cp.nProcessing = N;

	// create last thread (collector)
	hThreads[N] = CreateThread(NULL, 0, collectorThreadFunction, &cp, 0, NULL);
	if (hThreads[N] == NULL) {
		_ftprintf(stderr, _T("Impossible to create last thread. Error: %x"), GetLastError());
		for (i = 0; i < N; i++) {
			TerminateThread(hThreads[i], 1);
			CloseHandle(hThreads[i]);
		}
		free(hThreads);
		CloseHandle(hIn);
		return 7;
	}

	// wait for all of the processing threads
	if (WaitForVeryHugeNumberOfObjects(hThreads, N, TRUE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Impossible to wait for all the threads\n"));
		for (i = 0; i < N + 1; i++) {
			TerminateThread(hThreads[i], 1);
			CloseHandle(hThreads[i]);
		}
		free(hThreads);
		CloseHandle(hIn);
		return 8;
	}

	for (i = 0; i < N; i++) {
		CloseHandle(hThreads[i]);
	}
	CloseHandle(hIn);

	// wait for the collector thread
	if (WaitForSingleObject(hThreads[N], INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Impossible to wait for the last thread. Error: %x\n"), GetLastError());
		TerminateThread(hThreads[N], 1);
		CloseHandle(hThreads[N]);
		free(hThreads);
		return 9;
	}

	_tprintf(_T("Program going to terminate correctly\n"));
	CloseHandle(hThreads[N]);
	free(hThreads);

	return 0;
}

// named semaphores and mutexes can't have backslashes in the name
// so the backslashes are transformed into forward slashes (rotation)
//  - stringToManipulate: input and output (is modified)
//  - return value: success
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

// function executed by the processing threads
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
		// read a record from the recordsFile (in mutual exclusion)
		EnterCriticalSection(&pp->cs);
		result = ReadFile(pp->hRecordsFile, &record, sizeof(RECORD), &nRead, NULL);
		LeaveCriticalSection(&pp->cs);
		if (result == FALSE) {
			_ftprintf(stderr, _T("Impossible to read a record from the input file. Error: %x\n"), GetLastError());
			break;
		}
		if (nRead == 0) {
			// end of file
			// Sleep for 1 second so that i am sure that collector thread has already finished processing output files
			// because his WaitForMultiple gives no precedence to lower-indexed handles.
			// Anyway, if this time is not enough, the unprocessed output will be printed in the "purging phase" of
			// the collector.
			// But it seems to work also without sleeping, so i won't sleep
			//Sleep(1000);
			// signal on termination semaphore
			if (!ReleaseSemaphore(terminationSemaphore, 1, NULL)) {
				_ftprintf(stderr, _T("Impossible to release termination semaphore. Error: %x\n"), GetLastError());
				break;
			}
			// exit the loop
			break;
		}
		if (nRead != sizeof(RECORD)) {
			_ftprintf(stderr, _T("Record size mismatch\n"));
			break;
		}
		_tprintf(_T("Record read: %s %s %s\n"), record.directoryName, record.inputFileName, record.outputName);

		// initialize the outputRecord
		outputRecord.nFiles = 0;
		outputRecord.nChar = 0;
		outputRecord.nNewLines = 0;

		// do the visit that will update the outputRecord
		visitDirectoryAndDo(record.directoryName, record.inputFileName, 0, updateOutputRecord, &outputRecord);

		// prepare the names for the semaphore and mutex related to this output file
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
			break;
		}
		// Named Semaphore: CreateSemaphore returns the existing semaphore if it has already been created
		hSemaphoreOutput = CreateSemaphore(NULL, 0, MAX_SEM_VALUE, semaphoreOutputName);
		if (hSemaphoreOutput == NULL) {
			_ftprintf(stderr, _T("Impossible to create/open semaphore. Error: %x\n"), GetLastError());
			break;
		}

		// acquire the mutex for the output file (mutual exclusion of writers)
		if (WaitForSingleObject(hMutexOutput, INFINITE) == WAIT_FAILED) {
			_ftprintf(stderr, _T("Impossible to wait for the semaphore named %s. Error: %x"), mutexOutputName, GetLastError());
			break;
		}

		// open new/existing output file
		hFileOutput = CreateFile(record.outputName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFileOutput == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to open/create file %s. Error: %x\n"), record.outputName, GetLastError());
			ReleaseMutex(hMutexOutput);
			break;
		}
		// go to the end of file
		if (SetFilePointer(hFileOutput, 0, 0, FILE_END) == INVALID_SET_FILE_POINTER) {
			_ftprintf(stderr, _T("Impossible to set the file pointer to the end of the file %s. Error: %x\n"), record.outputName, GetLastError());
			ReleaseMutex(hMutexOutput);
			CloseHandle(hFileOutput);
			break;
		}
		// write the outputRecord
		if (!outputRecordWrite(hFileOutput, &outputRecord)) {
			_ftprintf(stderr, _T("Impossible to write output record. Error: %x\n"), GetLastError());
			ReleaseMutex(hMutexOutput);
			CloseHandle(hFileOutput);
			break;
		}
		// release the mutex for the file
		ReleaseMutex(hMutexOutput);
		// close the file handle (can't look-ahead to see if it will be used again)
		CloseHandle(hFileOutput);

		// Signal that a record has been written on the output file
		if (!ReleaseSemaphore(hSemaphoreOutput, 1, NULL)) {
			_ftprintf(stderr, _T("Impossible to release the semaphore for output file %s. Error: %x\n"), record.outputName, GetLastError());
			break;
		}
		// WARNING: don't close those handles, because no warranty that the collectorThreador other processingThreads 
		// already opened them. They are named objects, if i close them prematurely then the other threads will wait
		// forever because they will create effectively them again, instead of using existing.
		// Those handles will be automatically closed at the end of the process.
		//CloseHandle(hMutexOutput);
		//CloseHandle(hSemaphoreOutput);
	}
	// WARNING: also this handle must not be closed there, because processingThread may have not yet opened it, and
	// in this way it could loose the signal on the semaphore
	//CloseHandle(terminationSemaphore);

	return 0;
}

// function executed by the collector thread
DWORD WINAPI collectorThreadFunction(LPVOID p) {
	LPCOLLECTORPARAM cp = (LPCOLLECTORPARAM)p;
	HANDLE hRecordsFile;
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

	// create a handle for the input file, in order to fill the arrays
	hRecordsFile = CreateFile(cp->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hRecordsFile == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to open the records file. Error: %x\n"), GetLastError());
		return 2;
	}
	// read the file to get the output file names
	nOutputFiles = 0;
	while (TRUE) {
		// read a record
		if (!ReadFile(hRecordsFile, &record, sizeof(RECORD), &nRead, NULL)) {
			_ftprintf(stderr, _T("Impossible to read from records file. Error: %x\n"), GetLastError());
			CloseHandle(hRecordsFile);
			return 3;
		}
		if (nRead == 0) {
			// end of file
			break;
		}
		// start searching if this file was already found previously
		found = -1;
		for (i = 0; i < nOutputFiles; i++) {
			// comparison on the path
			if (_tcsncmp(outputFileNames[i], record.outputName, NAME_MAX_LEN) == 0) {
				// found
				found = i;
				break;
			}
		}
		if (found == -1) {
			// new path
			if (nOutputFiles >= MAX_OUTPUT_FILES) {
				_ftprintf(stderr, _T("Too many records in this file\n"));
				CloseHandle(hRecordsFile);
				return 4;
			}
			// save the path of this file
			_tcsncpy(outputFileNames[i], record.outputName, NAME_MAX_LEN);
			nOutputFiles++;
		}
	}
	// no more need of this file
	CloseHandle(hRecordsFile);

	// Now open all the semaphores for the output files and the output files

	for (i = 0; i < nOutputFiles; i++) {
		// generate the semaphore name following the same procedure as the processing threads
		_tcsncpy(semaphoreOutputName, PREFIX_NAME_SEM, NAME_MAX_LEN);
		_tcsncat(semaphoreOutputName, outputFileNames[i], NAME_MAX_LEN - PREFIX_NAME_LEN);
		rotateBackslashes(semaphoreOutputName);

		// create or open the semaphore
		hSemaphoresOutputAndTerminationSemaphore[i] = CreateSemaphore(NULL, 0, MAX_SEM_VALUE, semaphoreOutputName);
		if (hSemaphoresOutputAndTerminationSemaphore[i] == NULL) {
			_ftprintf(stderr, _T("Impossible to open semaphore for output file (collector). Error: %x\n"), GetLastError());
			return 1;
		}
		// initialize the counter of rows already there for processing
		rowsReady[i] = 0;
		// open the output file
		hOutputFiles[i] = CreateFile(outputFileNames[i], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOutputFiles[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to open output file %s. Error: %x"), outputFileNames[i], GetLastError());
			return 2;
		}
	}
	// the last handle of the array is the termination Semaphore (signalled by threads that reach end of records file)
	// in this way, a call on the modified WaitForMultipleObjects can return with a ready semaphore for output or for termination
	hSemaphoresOutputAndTerminationSemaphore[nOutputFiles] = CreateSemaphore(NULL, 0, cp->nProcessing, TERMINATION_SEMAPHORE_NAME);
	if (hSemaphoresOutputAndTerminationSemaphore[nOutputFiles] == NULL) {
		_ftprintf(stderr, _T("Impossible to open termination semaphore (collector). Error: %x\n"), GetLastError());
		return 1;
	}
	nTerminated = 0;

	// main cycle to wait some interesting things: output record added or some threads terminated
	while (nTerminated < cp->nProcessing) {
		result = WaitForVeryHugeNumberOfObjects(hSemaphoresOutputAndTerminationSemaphore, nOutputFiles + 1, FALSE);
		if (result == WAIT_FAILED) {
			_ftprintf(stderr, _T("Impossible to wait very huge number of objects (collector)\n"));
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
				// update rowsReady because they have been processed
				rowsReady[result] = 0;
			}
		}
	}
	// show the remaining parts of the files (if the number of records written to them are not multiple of M, or if
	// the files were not deleted after the previous run (they are still there)
	// PURGING PHASE
	_tprintf(_T("\nRemaining contents of output files:\n"));
	for (i = 0; i < nOutputFiles; i++) {
		_tprintf(_T("File %s:\n"), outputFileNames[i]);
		while (outputRecordRead(hOutputFiles[i], &outRecord)) {
			_tprintf(_T("\tn_files: %u\tn_char: %u\tn_newlines: %u\n"), outRecord.nFiles, outRecord.nChar, outRecord.nNewLines);
		}
		_tprintf(_T("END\n"));
		CloseHandle(hOutputFiles[i]);
		CloseHandle(hSemaphoresOutputAndTerminationSemaphore[i]);
	}
	CloseHandle(hSemaphoresOutputAndTerminationSemaphore[nOutputFiles]);
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
			// wait all the members of a chunk
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

VOID visitDirectoryAndDo(LPTSTR path1, LPTSTR search, DWORD level, VOID(*toDo)(LPTSTR path, LPOUTPUTRECORD outputRecord), LPOUTPUTRECORD outputRecord) {
	WIN32_FIND_DATA findFileData;
	HANDLE hFind;
	TCHAR searchPath[MAX_PATH];
	TCHAR newPath1[MAX_PATH];

	// build the searchPath string, to be able to search inside path1: searchPath = path1\*
	_sntprintf(searchPath, MAX_PATH - 1, _T("%s\\%s"), path1, search);
	searchPath[MAX_PATH - 1] = 0;

	// search inside path1
	hFind = FindFirstFile(searchPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("FindFirstFile failed. Error: %x\n"), GetLastError());
		return;
	}
	do {
		// generate a new path by appending to path1 the name of the found entry
		_sntprintf(newPath1, MAX_PATH, _T("%s\\%s"), path1, findFileData.cFileName);

		// check the type of file
		DWORD fType = FileType(&findFileData);
		if (fType == TYPE_FILE) {
			// this is a file
			//_tprintf(_T("FILE %s "), path1);
			toDo(newPath1, outputRecord);
		}
		if (fType == TYPE_DIR) {
			// this is a directory
			//_tprintf(_T("DIR %s\n"), path1);
			// recursive call to the new paths (in this exercise, no recursion)
			//visitDirectoryRecursiveAndDo(newPath1, level + 1, toDo, outputRecord);
		}
	} while (FindNextFile(hFind, &findFileData));

	FindClose(hFind);
	return;
}

static DWORD FileType(LPWIN32_FIND_DATA pFileData) {
	BOOL IsDir;
	DWORD FType;
	FType = TYPE_FILE;
	IsDir = (pFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (IsDir)
		if (lstrcmp(pFileData->cFileName, _T(".")) == 0 || lstrcmp(pFileData->cFileName, _T("..")) == 0)
			FType = TYPE_DOT;
		else FType = TYPE_DIR;
		return FType;
}

// function that the processing thread pass as argument of the visit function
VOID updateOutputRecord(LPTSTR path, LPOUTPUTRECORD outputRecord) {
	FILE *currentFile;
	TCHAR line[LINE_MAX_LEN];
	DWORD lineLength;
	// open for reading
	currentFile = _tfopen(path, _T("r"));
	if (currentFile == NULL) {
		_ftprintf(stderr, _T("Impossible to open file %s\n"), path);
		return;
	}
	// this is a new file
	outputRecord->nFiles++;
	// read a line
	while (_fgetts(line, LINE_MAX_LEN, currentFile) > 0) {
		lineLength = _tcsnlen(line, LINE_MAX_LEN);
		outputRecord->nNewLines++;
		outputRecord->nChar += lineLength;
	}
	fclose(currentFile);
	return;
}
