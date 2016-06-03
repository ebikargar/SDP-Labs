/*
Lab 11 exercise 01

This program performs a multithread update of an accounts file,
by updating the amounts stored in the records by considering some
operations that are stored on other files. Each thread is
responsible for an operations file. The accounts file is protected
for concurrent access by using different techniques:
- version A: file locking on the single record
- version B: critical section (locking the access to the whole file)
- version C: mutex (locking the access to the whole file)
- version B: semaphore (locking the access to the whole file)

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

//------------------------------------------------------
// simply change this define to 'A', 'B', 'C', or 'D'
#define VERSION 'D'
//------------------------------------------------------

#if VERSION == 'A'
#define initializeSync initializeSyncA
#define initThread initThreadReal
#define acquireSync acquireSyncA
#define releaseSync releaseSyncA
#endif // VERSION == 'A'
#if VERSION == 'B'
#define initializeSync initializeSyncB
#define initThread initThreadFake
#define acquireSync acquireSyncB
#define releaseSync releaseSyncB
#endif // VERSION == 'B'
#if VERSION == 'C'
#define initializeSync initializeSyncC
#define initThread initThreadFake
#define acquireSync acquireSyncC
#define releaseSync releaseSyncC
#endif // VERSION == 'C'
#if VERSION == 'D'
#define initializeSync initializeSyncD
#define initThread initThreadFake
#define acquireSync acquireSyncD
#define releaseSync releaseSyncD
#endif // VERSION == 'D'


#define NAME_MAX_LEN 30 + 1

typedef struct _record {
	DWORD id;
	DWORD account_number;
	TCHAR surname[NAME_MAX_LEN];
	TCHAR name[NAME_MAX_LEN];
	INT amount;
} RECORD;

typedef RECORD* LPRECORD;

typedef union _sync_obj {
	HANDLE h;	// to a mutex or semaphore (shared by threads) / file (different for each thread)
	LPCRITICAL_SECTION cs; // shared by threads
} SYNC_OBJ;

typedef SYNC_OBJ* LPSYNC_OBJ;

typedef struct _param {
	HANDLE hAccounts;
	LPTSTR accountsFileName;
	LPTSTR operationsFileName;
	SYNC_OBJ sync;	// this is copied, not passed by reference, so each thread can overwrite (version A overwrites h)
} PARAM;

typedef PARAM* LPPARAM;

BOOL initializeSyncA(LPSYNC_OBJ);
BOOL initializeSyncB(LPSYNC_OBJ);
BOOL initializeSyncC(LPSYNC_OBJ);
BOOL initializeSyncD(LPSYNC_OBJ);
HANDLE openAccountsFile(LPTSTR);
BOOL initThreadReal(LPPARAM);
BOOL initThreadFake(LPPARAM);
BOOL acquireSyncA(LPSYNC_OBJ, OVERLAPPED);
BOOL acquireSyncB(LPSYNC_OBJ, OVERLAPPED);
BOOL acquireSyncC(LPSYNC_OBJ, OVERLAPPED);
BOOL acquireSyncD(LPSYNC_OBJ, OVERLAPPED);
BOOL releaseSyncA(LPSYNC_OBJ, OVERLAPPED);
BOOL releaseSyncB(LPSYNC_OBJ, OVERLAPPED);
BOOL releaseSyncC(LPSYNC_OBJ, OVERLAPPED);
BOOL releaseSyncD(LPSYNC_OBJ, OVERLAPPED);
DWORD WINAPI doOperations(LPVOID);

BOOL displayRecords(HANDLE hFile);
VOID freeSync(LPSYNC_OBJ sync);
VOID teardownThread(LPPARAM param);


INT _tmain(INT argc, LPTSTR argv[]) {
	HANDLE hAccounts = NULL;
	DWORD nThreads;
	LPPARAM params = NULL;
	SYNC_OBJ sync;	// depending on version, can be nothing, a handle (mutex or semaphore) or a critical section
	LPHANDLE tHandles = NULL;
	DWORD i;

	// check number of parameters
	if (argc <= 2) {
		_ftprintf(stderr, _T("Usage: %s accounts_file list_of_operations_files\n"), argv[0]);
		return 1;
	}

	// initialize the syncronization mechanism
	if (!initializeSync(&sync)) {
		_ftprintf(stderr, _T("Error initializing the sync object\n"));
		return 2;
	}
	// open the accounts file
	hAccounts = openAccountsFile(argv[1]);
	if (hAccounts == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Error opening the accounts file\n"));
		freeSync(&sync);
		return 3;
	}
	assert(hAccounts);
	_tprintf(_T("This is the content of the accounts file before processing the operations:\n"));
	if (!displayRecords(hAccounts)) {
		CloseHandle(hAccounts);
		freeSync(&sync);
		return 4;
	}

	nThreads = argc - 2;
	// allocate arrays for threads
	params = (LPPARAM)calloc(nThreads, sizeof(PARAM));
	tHandles = (LPHANDLE)calloc(nThreads, sizeof(HANDLE));
	if (params == NULL || tHandles == NULL) {
		_ftprintf(stderr, _T("Error allocating some arrays for threads\n"));
		CloseHandle(hAccounts);
		freeSync(&sync);
		return 5;
	}

	// for each operation file, create a thread
	for (i = 0; i < nThreads; i++) {
		params[i].hAccounts = hAccounts;
		params[i].accountsFileName = argv[1];
		params[i].operationsFileName = argv[i + 2];
		// the sync union value is copied, so that version A threads can have a different handle.
		// Versions C and D will have different copies of the same HANDLE (it is a pointer, so
		// the semaphore and the mutex will be shared. Version B has a pointer to a critical section
		// that is shared.
		params[i].sync = sync;
		tHandles[i] = CreateThread(NULL, 0, doOperations, &params[i], 0, NULL);
		if (tHandles[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Error creating a thread\n"));
			CloseHandle(hAccounts);
			freeSync(&sync);
			free(params);
			free(tHandles);
			return 6;
		}
		assert(tHandles[i]);
	}

	if (WaitForMultipleObjects(nThreads, tHandles, TRUE, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Impossible to wait the threads. Error: %x\n"), GetLastError());
		return 7;
	}
	for (i = 0; i < nThreads; i++) {
		CloseHandle(tHandles[i]);
	}
	free(params);
	free(tHandles);
	freeSync(&sync);

	_tprintf(_T("This is the content of the accounts file after processing the operations:\n"));
	displayRecords(hAccounts);
	CloseHandle(hAccounts);
	return 0;
}

DWORD WINAPI doOperations(LPVOID p) {
	LPPARAM param = (LPPARAM)p;
	HANDLE hOperations;
	RECORD operationRecord, accountRecord;
	DWORD nRead;

	initThread(param);

	// open the binary operations file for reading
	hOperations = CreateFile(param->operationsFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	// check the HANDLE value
	if (hOperations == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open operations file %s. Error: %x\n"), param->operationsFileName, GetLastError());
		teardownThread(param);
		return 1;
	}
	while (ReadFile(hOperations, &operationRecord, sizeof(operationRecord), &nRead, NULL) && nRead > 0) {
		if (nRead != sizeof(operationRecord)) {
			_ftprintf(stderr, _T("Record size mismatch on file %s!\n"), param->operationsFileName);
			CloseHandle(hOperations);
			teardownThread(param);
			return 2;
		}
		// create a "clean" OVERLAPPED structure
		OVERLAPPED ov = { 0, 0, 0, 0, NULL };
		// the index should be 0-based because at the beginning of the file the record with id = 1 is stored
		LONGLONG n = operationRecord.id - 1;
		LARGE_INTEGER FilePos;
		// the displacement in the file is obtained by multiplying the index by the size of a single element
		FilePos.QuadPart = n * sizeof(operationRecord);
		// copy the displacement inside the OVERLAPPED structure
		ov.Offset = FilePos.LowPart;
		ov.OffsetHigh = FilePos.HighPart;

		// require the sync object
		if (!acquireSync(&param->sync, ov)) {
			CloseHandle(hOperations);
			teardownThread(param);
			return 3;
		}

		// do stuff there
		if (!ReadFile(param->hAccounts, &accountRecord, sizeof(accountRecord), &nRead, &ov) || nRead != sizeof(accountRecord)) {
			_ftprintf(stderr, _T("Error reading record\n"));
			releaseSync(&param->sync, ov);
			CloseHandle(hOperations);
			teardownThread(param);
			return 4;
		}
		_tprintf(_T("Account with id %u: amount: %d operation with amount: %d --> new amount: %d\n"), accountRecord.id, accountRecord.amount, operationRecord.amount, accountRecord.amount + operationRecord.amount);
		accountRecord.amount += operationRecord.amount;
		if (!WriteFile(param->hAccounts, &accountRecord, sizeof(accountRecord), &nRead, &ov) || nRead != sizeof(accountRecord)) {
			_ftprintf(stderr, _T("Error writing record\n"));
			releaseSync(&param->sync, ov);
			CloseHandle(hOperations);
			teardownThread(param);
			return 5;
		}
		// release
		releaseSync(&param->sync, ov);
	}
	CloseHandle(hOperations);
	teardownThread(param);
	return 0;
}

HANDLE openAccountsFile(LPTSTR path) {
	// open the binary file for reading/writing shared
	// error checking is done by the caller
	return CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

// this function is called only by version A
BOOL initThreadReal(LPPARAM param) {
	// open the binary file for reading/writing
	param->hAccounts = openAccountsFile(param->accountsFileName);
	// check the HANDLE value
	if (param->hAccounts == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open accounts file %s. Error: %x\n"), param->accountsFileName, GetLastError());
		return FALSE;
	}

	// copy into the handle the file handle, required for locking/unlocking the file
	param->sync.h = param->hAccounts;

	return TRUE;
}

BOOL initThreadFake(LPPARAM param) {
	// do nothing, accounts file is opened by main
	return TRUE;
}


BOOL initializeSyncA(LPSYNC_OBJ) {
	// do nothing, syncronization is done by file locking, that needs no initialization
	return TRUE;
}
BOOL acquireSyncA(LPSYNC_OBJ sync, OVERLAPPED o) {
	LARGE_INTEGER recordSize;
	recordSize.QuadPart = sizeof(RECORD);
	return LockFileEx(sync->h, LOCKFILE_EXCLUSIVE_LOCK, 0, recordSize.LowPart, recordSize.HighPart, &o);
}
BOOL releaseSyncA(LPSYNC_OBJ sync, OVERLAPPED o) {
	LARGE_INTEGER recordSize;
	recordSize.QuadPart = sizeof(RECORD);
	return UnlockFileEx(sync->h, 0, recordSize.LowPart, recordSize.HighPart, &o);
}

BOOL initializeSyncB(LPSYNC_OBJ sync) {
	sync->cs = (LPCRITICAL_SECTION)calloc(1, sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(sync->cs);
	return TRUE;
}
BOOL acquireSyncB(LPSYNC_OBJ sync, OVERLAPPED o) {
	EnterCriticalSection(sync->cs);
	return TRUE;
}
BOOL releaseSyncB(LPSYNC_OBJ sync, OVERLAPPED o) {
	LeaveCriticalSection(sync->cs);
	return TRUE;
}

BOOL initializeSyncC(LPSYNC_OBJ sync) {
	sync->h = CreateMutex(NULL, FALSE, NULL);
	if (sync->h == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	return TRUE;
}
BOOL acquireSyncC(LPSYNC_OBJ sync, OVERLAPPED o) {
	if (WaitForSingleObject(sync->h, INFINITE) == WAIT_FAILED) {
		return FALSE;
	}
	return TRUE;
}
BOOL releaseSyncC(LPSYNC_OBJ sync, OVERLAPPED o) {
	return ReleaseMutex(sync->h);
}

BOOL initializeSyncD(LPSYNC_OBJ sync) {
	sync->h = CreateSemaphore(NULL, 1, 1, NULL);
	if (sync->h == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	return TRUE;
}
BOOL acquireSyncD(LPSYNC_OBJ sync, OVERLAPPED o) {
	if (WaitForSingleObject(sync->h, INFINITE) == WAIT_FAILED) {
		return FALSE;
	}
	return TRUE;
}
BOOL releaseSyncD(LPSYNC_OBJ sync, OVERLAPPED o) {
	return ReleaseSemaphore(sync->h, 1, NULL);
}

// this function does not modify the file pointer
BOOL displayRecords(HANDLE hFile) {
	RECORD r;
	DWORD nRead;

	LARGE_INTEGER zero = { 0 };
	LARGE_INTEGER savedFilePointer = { 0 };
	SetFilePointerEx(hFile, zero, &savedFilePointer, FILE_CURRENT);

	// go to the beginning of the file
	SetFilePointerEx(hFile, zero, NULL, FILE_BEGIN);

	while (ReadFile(hFile, &r, sizeof(r), &nRead, NULL) && nRead > 0) {
		if (nRead != sizeof(r)) {
			_ftprintf(stderr, _T("Record size mismatch!\n"));
			// restore the file pointer
			SetFilePointerEx(hFile, savedFilePointer, NULL, FILE_BEGIN);
			return FALSE;
		}
		_tprintf(_T("%u %u %s %s %d\n"), r.id, r.account_number, r.surname, r.name, r.amount);
	}
	// restore the file pointer
	SetFilePointerEx(hFile, savedFilePointer, NULL, FILE_BEGIN);
	return TRUE;
}

VOID freeSync(LPSYNC_OBJ sync) {
#if VERSION == 'C' || VERSION == 'D'
	CloseHandle(sync->h);
#endif // VERSION == 'C' || VERSION == 'D'
#if VERSION == 'B'
	DeleteCriticalSection(sync->cs);
	free(sync->cs);
#endif // VERSION == 'B'
}

VOID teardownThread(LPPARAM param) {
#if VERSION == 'A'
	CloseHandle(param->hAccounts);
#endif // VERSION == 'A'
}