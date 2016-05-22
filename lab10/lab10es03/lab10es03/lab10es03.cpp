/*
Lab 10 exercise 03

This program performs a parallel visit of the folders specified as
parameters on the command line comparing them. Upon termination the
program has to state whether all directory have the same content or
not.

@Author: Martino Mensio
*/

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS

// DEBUG set to 1 allows a verbose output
#define DEBUG 1

#include <Windows.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>

#define TYPE_FILE 1
#define TYPE_DIR 2
#define TYPE_DOT 3

// name rules for semaphores
#define readerSemaphoreNameTemplate _T("lab10es03_ReaderSemaphore")
#define comparatorSemaphoreName _T("lab10es03_ComparatorSemaphore")

// structure that will be passed as parameter to a reader thread
typedef	struct _readerparam {
	volatile BOOL terminate;	// can be set by comparator
	volatile BOOL done;			// can be set by reader
	LPHANDLE readerSemaphore;	// reader waits on it
	LPHANDLE comparatorSemaphore;	// comparator waits on it
	TCHAR entry[MAX_PATH];		// the path to be compared
	LPTSTR path;				// the initial path to explore, is used also to compare entries (this part is discarded)
} READERPARAM;

typedef READERPARAM* LPREADERPARAM;

// structure that will be passed as parameter to the comparator thread
typedef	struct _comparatorparam {
	DWORD nReaders;
	LPREADERPARAM params;
	LPHANDLE comparatorSemaphore;
	LPHANDLE readersSemaphore;
} COMPARATORPARAM;

typedef COMPARATORPARAM* LPCOMPARATORPARAM;

BOOL visitDirectoryRAndDo(LPREADERPARAM param, LPTSTR path, DWORD level, BOOL(*toDo)(LPREADERPARAM, LPTSTR));
static DWORD FileType(LPWIN32_FIND_DATA pFileData);
DWORD WINAPI comparatorThreadFunction(LPVOID param);
DWORD WINAPI readerThreadFunction(LPVOID param);
LPTSTR getEntryPartialPath(LPREADERPARAM param);
BOOL whatToDo(LPREADERPARAM param, LPTSTR entry);

INT _tmain(INT argc, LPTSTR argv[]) {
	LPHANDLE readersHandles;
	HANDLE comparatorHandle;
	//LPDWORD threadIds;
	DWORD nReaders;
	DWORD i;
	LPREADERPARAM readerParams;
	COMPARATORPARAM comparatorParam;

	HANDLE comparatorSemaphore;
	LPHANDLE readersSemaphore;

	if (argc < 2) {
		_ftprintf(stderr, _T("Usage: %s list_of_folders_to_be_visited\n"), argv[0]);
		return 1;
	}
	nReaders = argc - 1;
	readersHandles = (LPHANDLE)malloc(nReaders * sizeof(HANDLE));
	if (readersHandles == NULL) {
		_ftprintf(stderr, _T("Error allocating space for threads\n"));
		return 2;
	}
	readersSemaphore = (LPHANDLE)malloc(nReaders * sizeof(HANDLE));
	if (readersSemaphore == NULL) {
		_ftprintf(stderr, _T("Error allocating the semaphores\n"));
	}
	comparatorSemaphore = CreateSemaphore(NULL, 0, nReaders, comparatorSemaphoreName);
	if (readersSemaphore == INVALID_HANDLE_VALUE || comparatorSemaphore == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Error Creating the semaphores\n"));
		return 3;
	}

	readerParams = (LPREADERPARAM)malloc(nReaders * sizeof(READERPARAM));
	if (readerParams == NULL) {
		_ftprintf(stderr, _T("Error allocating params\n"));
		return 4;
	}

	for (i = 0; i < nReaders; i++) {
		TCHAR semaphoreName[MAX_PATH];
		_stprintf(semaphoreName, _T("%s%d"), readerSemaphoreNameTemplate, i);
		readersSemaphore[i] = CreateSemaphore(NULL, 1, 1, semaphoreName);
		if (readersSemaphore[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Error Creating a semaphore\n"));
			return 5;
		}

		readerParams[i].comparatorSemaphore = &comparatorSemaphore;
		readerParams[i].readerSemaphore = &readersSemaphore[i];
		readerParams[i].done = FALSE;
		readerParams[i].terminate = FALSE;
		//readerParams[i].entry = NULL;
		readerParams[i].path = argv[i + 1];
		
		readersHandles[i] = CreateThread(0, 0, readerThreadFunction, &readerParams[i], 0, NULL);
		if (readersHandles[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Error creating reader thread\n"));
			return 6;
		}
	}

	comparatorParam.nReaders = nReaders;
	comparatorParam.params = readerParams;
	comparatorParam.comparatorSemaphore = &comparatorSemaphore;
	comparatorParam.readersSemaphore = readersSemaphore;

	comparatorHandle = CreateThread(NULL, 0, comparatorThreadFunction, &comparatorParam, 0, NULL);
	if (comparatorHandle == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Error creating comparator thread\n"));
		return 5;
	}

	WaitForMultipleObjects(nReaders, readersHandles, TRUE, INFINITE);
	for (i = 0; i < nReaders; i++) {
		CloseHandle(readersHandles[i]);
	}
	free(readersHandles);

	WaitForSingleObject(comparatorHandle, INFINITE);
	CloseHandle(comparatorHandle);
	return 0;

}

DWORD WINAPI readerThreadFunction(LPVOID param) {
	LPREADERPARAM readerParam = (LPREADERPARAM)param;
	BOOL earlyTermination;

	// the wait must be done inside the toDo function (read entry)
	//WaitForSingleObject(readerParam->readersSemaphore, INFINITE);
	
	earlyTermination = !visitDirectoryRAndDo(readerParam, readerParam->path, 0, whatToDo);

	// the visit finished
#if DEBUG
	_tprintf(_T("reader: finished. Early termination: %d\n"), earlyTermination);
#endif // DEBUG
	if (earlyTermination) {
		// TODO this should be not necessary, after fix on semaphores
		//WaitForSingleObject(*readerParam->readerSemaphore, INFINITE);
		readerParam->done = 1;
		ReleaseSemaphore(*readerParam->comparatorSemaphore, 1, NULL);
		return 0;
	} else {
		WaitForSingleObject(*readerParam->readerSemaphore, INFINITE);
		readerParam->done = 1;
		ReleaseSemaphore(*readerParam->comparatorSemaphore, 1, NULL);
		return 0;
	}
}

DWORD WINAPI comparatorThreadFunction(LPVOID param) {
	LPCOMPARATORPARAM comparatorParam = (LPCOMPARATORPARAM)param;
	DWORD i;
	BOOL someContinue, someFinished, different;
	LPTSTR strToCompare;

	different = FALSE;
	someContinue = TRUE;

	while (!different && someContinue) {
		someContinue = someFinished = FALSE;
		different = FALSE;
		strToCompare = NULL;
		// wait nReader times on comparator semaphore
		//_tprintf(_T("comparator: waiting on comparator semaphore\n"));
		for (i = 0; i < comparatorParam->nReaders; i++) {
			WaitForSingleObject(*comparatorParam->comparatorSemaphore, INFINITE);
		}
		//WaitForMultipleObjects(comparatorParam->nReaders, comparatorParam->readersSemaphore, TRUE, INFINITE);
		// compare
		//_tprintf(_T("comparator: comparing\n"));
		for (i = 0; i < comparatorParam->nReaders; i++) {
			if (comparatorParam->params[i].done) {
				someFinished = TRUE;
			} else {
				someContinue = TRUE;
				if (strToCompare == NULL) {
#if DEBUG
					_tprintf(_T("comparator: first entry:   %s\n"), comparatorParam->params[i].entry);
#endif // DEBUG
					strToCompare = getEntryPartialPath(&comparatorParam->params[i]);
					//strToCompare = comparatorParam->params[i].entry;
				} else {
#if DEBUG
					_tprintf(_T("comparator: another entry: %s\n"), comparatorParam->params[i].entry);
#endif // DEBUG
					if (_tcsncmp(strToCompare, getEntryPartialPath(&comparatorParam->params[i]), MAX_PATH) != 0) {
					//if (_tcsncmp(strToCompare, comparatorParam->params[i].entry, MAX_PATH)) {
						different = TRUE;
						break;
					}
				}
			}
		}
		if (someFinished && someContinue) {
#if DEBUG
			_tprintf(_T("comparator: some finished and some continue\n"));
#endif // DEBUG
			different = TRUE;
		}
		if (different) {
#if DEBUG
			_tprintf(_T("comparator: different\n"));
#endif // DEBUG
			for (i = 0; i < comparatorParam->nReaders; i++) {
				comparatorParam->params[i].terminate = TRUE;
			}
		}
		// signal nReader times on reader semaphore
		//ReleaseSemaphore(*comparatorParam->readersSemaphore, comparatorParam->nReaders, NULL);
		for (i = 0; i < comparatorParam->nReaders; i++) {
			ReleaseSemaphore(comparatorParam->readersSemaphore[i], 1, NULL);
		}
	}
	_tprintf(_T("Comparator result: the subtrees are %s\n"), different ? _T("different") : _T("equal"));
	return 0;
}

BOOL whatToDo(LPREADERPARAM param, LPTSTR entry) {
	//_tprintf(_T("reader: Waiting on readers semaphore\n"));
	WaitForSingleObject(*param->readerSemaphore, INFINITE);
#if DEBUG
	_tprintf(_T("reader: entry: %s\n"), entry);
#endif // DEBUG
	_tcsncpy(param->entry, entry, MAX_PATH);
	//_tprintf(_T("reader: Releasing the comparator semaphore\n"));
	ReleaseSemaphore(*param->comparatorSemaphore, 1, NULL);
	if (param->terminate) {
#if DEBUG
		_tprintf(_T("reader: they told me to terminate\n"));
#endif // DEBUG
		return FALSE;
	} else {
		return TRUE;
	}
}

BOOL visitDirectoryRAndDo(LPREADERPARAM param, LPTSTR path, DWORD level, BOOL(*toDo)(LPREADERPARAM, LPTSTR)) {
	//LPTSTR path1 = param->path;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind;
	TCHAR searchPath[MAX_PATH];
	TCHAR newPath[MAX_PATH];

	// build the searchPath string, to be able to search inside path1: searchPath = path1\*
	_sntprintf(searchPath, MAX_PATH - 1, _T("%s\\*"), path);

	// search inside path1
	hFind = FindFirstFile(searchPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("FindFirstFile failed. Error: %x\n"), GetLastError());
		// return true so can continue on other subtrees
		return FALSE;
	}
	do {
		// generate a new path by appending to path the name of the found entry
		_sntprintf(newPath, MAX_PATH, _T("%s\\%s"), path, findFileData.cFileName);
		if (!toDo(param, newPath)) {
			return FALSE;
		}
		// check the type of file
		DWORD fType = FileType(&findFileData);
		if (fType == TYPE_FILE) {
			// this is a file
			//_tprintf(_T("FILE %s "), path1);
			// call the function to copy with modifications
		}
		if (fType == TYPE_DIR) {
			// this is a directory
			//_tprintf(_T("DIR %s\n"), path1);
			// recursive call to the new paths
			if (!visitDirectoryRAndDo(param, newPath, level + 1, toDo)) {
				return FALSE;
			}
		}
	} while (FindNextFile(hFind, &findFileData));

	FindClose(hFind);
	return TRUE;
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

LPTSTR getEntryPartialPath(LPREADERPARAM param) {
	DWORD pathLen;
	pathLen = (DWORD)_tcsnlen(param->path, MAX_PATH);
	return &param->entry[pathLen];
}
