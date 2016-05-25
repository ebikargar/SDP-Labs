/*
Lab 10 exercise 03

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

#define VERSION 'A'

#if VERSION == 'A'
#define initializeSync initializeSyncA
#define openAccountsFile openAccountsFileFake
#define initThread initThreadReal
#define acquireSync acquireSyncA
#define releaseSync releaseSyncA
#endif // VERSION == 'A'
#if VERSION == 'B'
#define initializeSync initializeSyncB
#define openAccountsFile openAccountsFileReal
#define initThread initThreadFake
#define acquireSync acquireSyncB
#define releaseSync releaseSyncC
#endif // VERSION == 'B'
#if VERSION == 'C'
#define initializeSync initializeSyncC
#define openAccountsFile openAccountsFileReal
#define initThread initThreadFake
#define acquireSync acquireSyncC
#define releaseSync releaseSyncC
#endif // VERSION == 'C'
#if VERSION == 'D'
#define initializeSync initializeSyncD
#define openAccountsFile openAccountsFileReal
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

typedef struct _param {
	HANDLE hAccounts;
	LPTSTR accountsFileName;
	LPTSTR operationsFileName;
} PARAM;

typedef PARAM* LPPARAM;

typedef union _sync_obj {
	HANDLE h;	// to a mutex / semaphore
	CRITICAL_SECTION cs;
	// or nothing
} SYNC_OBJ;

typedef SYNC_OBJ* LPSYNC_OBJ;

BOOL initializeSyncA(LPSYNC_OBJ);
BOOL initializeSyncB(LPSYNC_OBJ);
BOOL initializeSyncC(LPSYNC_OBJ);
BOOL initializeSyncD(LPSYNC_OBJ);
HANDLE openAccountsFileFake(LPTSTR);
HANDLE openAccountsFileReal(LPTSTR);
BOOL initThreadReal(LPPARAM);
BOOL initThreadFake(LPPARAM);
BOOL acquireSyncA(HANDLE, OVERLAPPED);
BOOL acquireSyncB(HANDLE, OVERLAPPED);
BOOL acquireSyncC(HANDLE, OVERLAPPED);
BOOL acquireSyncD(HANDLE, OVERLAPPED);
BOOL releaseSyncA(HANDLE, OVERLAPPED);
BOOL releaseSyncB(HANDLE, OVERLAPPED);
BOOL releaseSyncC(HANDLE, OVERLAPPED);
BOOL releaseSyncD(HANDLE, OVERLAPPED);
DWORD WINAPI doOperations(LPVOID);


INT _tmain(INT argc, LPTSTR argv[]) {
	HANDLE hAccounts = NULL;
	DWORD nThreads;
	LPPARAM params;
	SYNC_OBJ sync;	// depending on version, can be nothing, a handle (mutex or semaphore) or a critical section
	LPHANDLE tHandles;
	DWORD i;

	// check number of parameters
	if (argc <= 2) {
		_ftprintf(stderr, _T("Usage: %s accounts_file list_of_operations_files\n"), argv[0]);
		return 1;
	}

	// versions different from A need to open the fileHandle accounts and initialize the sync object
#if VERSION != 'A'
	
#endif // VERSION != 'A'

	// initialize the syncronization mechanism
	if (!initializeSync(&sync)) {
		_ftprintf(stderr, _T("Error initializing the sync object\n"));
		return 2;
	}
	hAccounts = openAccountsFile(argv[1]);
	if (hAccounts == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Error opening the accounts file\n"));
		return 3;
	}
	

	nThreads = argc - 2;
	params = (LPPARAM)malloc(nThreads * sizeof(PARAM));
	tHandles = (LPHANDLE)malloc(nThreads * sizeof(HANDLE));
	if (params == NULL || tHandles == NULL) {
		_ftprintf(stderr, _T("Error allocating some arrays for threads\n"));
		if (hAccounts) {
			CloseHandle(hAccounts);
		}
		return 3;
	}

	// for each operation file, do the work
	for (i = 0; i < nThreads; i++) {
		params[i].hAccounts = hAccounts;
		params[i].accountsFileName = argv[1];
		params[i].operationsFileName = argv[i + 2];
		tHandles[i] = CreateThread(NULL, 0, doOperations, &params[i], 0, NULL);
		if (tHandles[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Error creating a thread\n"));
			if (hAccounts) {
				CloseHandle(hAccounts);
			}
			free(params);
			free(tHandles);
			return 4;
		}
	}

	WaitForMultipleObjects(nThreads, tHandles, TRUE, INFINITE);
	for (i = 0; i < nThreads; i++) {
		CloseHandle(tHandles[i]);
	}

	if (hAccounts) {
		CloseHandle(hAccounts);
	}
	free(params);
	free(tHandles);
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
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"), param->operationsFileName, GetLastError());
		return 2;
	}
	while (ReadFile(hOperations, &operationRecord, sizeof(operationRecord), &nRead, NULL) && nRead > 0) {
		if (nRead != sizeof(operationRecord)) {
			_ftprintf(stderr, _T("Record size mismatch on file %s!\n"), param->operationsFileName);
			CloseHandle(hOperations);
			return 3;
		}
		// create a "clean" OVERLAPPED structure
		OVERLAPPED ov = { 0, 0, 0, 0, NULL };
		// the index should be 0-based because at the beginning of the file the student with id = 1 is stored
		LONGLONG n = operationRecord.id - 1;
		LARGE_INTEGER FilePos;
		// the displacement in the file is obtained by multiplying the index by the size of a single element
		FilePos.QuadPart = n * sizeof(operationRecord);
		// copy the displacement inside the OVERLAPPED structure
		ov.Offset = FilePos.LowPart;
		ov.OffsetHigh = FilePos.HighPart;

		// require the sync object
		acquireSyncA(param->hAccounts, ov);

		// do stuff there
		if (!ReadFile(param->hAccounts, &accountRecord, sizeof(accountRecord), &nRead, &ov) || nRead != sizeof(accountRecord)) {
			_ftprintf(stderr, _T("Error reading record\n"));
			// TODO release resources
			return 4;
		}
		_tprintf(_T("Account with id %u: amount: %d found operation with amount: %d\n"), accountRecord.id, accountRecord.amount, operationRecord.amount);
		accountRecord.amount += operationRecord.amount;
		if (!WriteFile(param->hAccounts, &accountRecord, sizeof(accountRecord), &nRead, &ov) || nRead != sizeof(accountRecord)) {
			_ftprintf(stderr, _T("Error writing record\n"));
			// TODO release resources
			return 5;
		}
		// release
		releaseSyncA(param->hAccounts, ov);
	}
	return 0;
}

HANDLE openAccountsFileFake(LPTSTR) {
	// do nothing, accounts file is opened by threads
	return NULL;
}

BOOL initThreadReal(LPPARAM param) {
	// open the binary file for reading/writing
	param->hAccounts = CreateFile(param->accountsFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	// check the HANDLE value
	if (param->hAccounts == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"), param->accountsFileName, GetLastError());
		return FALSE;
	}
	return TRUE;
}

HANDLE openAccountsFileReal(LPTSTR path) {
	// open the binary file for reading/writing
	// error checking is done by the caller
	return CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

BOOL initThreadFake(LPPARAM param) {
	// do nothing, accounts file is opened by main
	return TRUE;
}


BOOL initializeSyncA(LPSYNC_OBJ) {
	// do nothing, syncronization is done by file locking, that needs no initialization
	return TRUE;
}
BOOL acquireSyncA(HANDLE h, OVERLAPPED o) {
	LARGE_INTEGER recordSize;
	recordSize.QuadPart = sizeof(RECORD);
	return LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK, 0, recordSize.LowPart, recordSize.HighPart, &o);
}
BOOL releaseSyncA(HANDLE h, OVERLAPPED o) {
	LARGE_INTEGER recordSize;
	recordSize.QuadPart = sizeof(RECORD);
	return UnlockFileEx(h, 0, recordSize.LowPart, recordSize.HighPart, &o);
}

BOOL initializeSyncB(LPSYNC_OBJ sync) {
	InitializeCriticalSection(&sync->cs);
	return TRUE;
}

BOOL acquireSyncB(HANDLE h, OVERLAPPED o) {
	LARGE_INTEGER recordSize;
	recordSize.QuadPart = sizeof(RECORD);
	return LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK, 0, recordSize.LowPart, recordSize.HighPart, &o);
}

BOOL releaseSyncB(HANDLE h, OVERLAPPED o) {
	LARGE_INTEGER recordSize;
	recordSize.QuadPart = sizeof(RECORD);
	return UnlockFileEx(h, 0, recordSize.LowPart, recordSize.HighPart, &o);
}