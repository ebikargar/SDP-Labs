/*
Lab 10 exercise 02

This program performs a parallel visit of the folders specified as
parameters on the command line.

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
#include <assert.h>
#include <stdio.h>

// The version can be set to 'A', 'B' or 'C'
#define VERSION 'C'
//---------------------------------
#if VERSION == 'A'
#define visitDirectory visitDirectoryA
#define whatToDo whatToDoA
#define collectData collectDataA
#endif // VERSION == 'A'
#if VERSION == 'B'
#define visitDirectory visitDirectoryB
#define whatToDo whatToDoB
#define collectData collectDataB
#endif // VERSION == 'B'
#if VERSION == 'C'
#define visitDirectory visitDirectoryC
#define whatToDo whatToDoC
#define collectData collectDataC
#endif // VERSION == 'C'

CRITICAL_SECTION cs;

#define TYPE_FILE 1
#define TYPE_DIR 2
#define TYPE_DOT 3

DWORD WINAPI visitDirectoryA(LPVOID param);
DWORD WINAPI visitDirectoryB(LPVOID param);
DWORD WINAPI visitDirectoryC(LPVOID param);
VOID whatToDoA(LPTSTR path, LPTSTR entryName, HANDLE fHandle);
VOID whatToDoB(LPTSTR path, LPTSTR entryName, HANDLE fHandle);
VOID whatToDoC(LPTSTR path, LPTSTR entryName, HANDLE fHandle);
VOID collectDataA(DWORD index);
VOID collectDataB(DWORD index);
VOID collectDataC(DWORD index);

VOID visitDirectoryRAndDo(LPTSTR path1, DWORD level, VOID(*toDo)(LPTSTR path, LPTSTR entryName, HANDLE fHandle), HANDLE fHandle);
static DWORD FileType(LPWIN32_FIND_DATA pFileData);
LPTSTR getFileNameByThreadId(DWORD tId);

INT _tmain(INT argc, LPTSTR argv[]) {
	LPHANDLE tHandles;
	LPDWORD threadIds;
	DWORD nThreads;
	DWORD i;

	if (argc < 2) {
		_ftprintf(stderr, _T("Usage: %s list_of_folders_to_be_visited\n"), argv[0]);
		return 1;
	}
	nThreads = argc - 1;
	tHandles = (LPHANDLE)malloc(nThreads * sizeof(HANDLE));
	threadIds = (LPDWORD)malloc(nThreads * sizeof(DWORD));
	if (tHandles == NULL || threadIds == NULL) {
		_ftprintf(stderr, _T("Error allocating space for threads\n"));
		return 2;
	}
	InitializeCriticalSection(&cs);
	for (i = 0; i < nThreads; i++) {
		tHandles[i] = CreateThread(0, 0, visitDirectory, argv[i + 1], 0, &threadIds[i]);
		if (tHandles[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Error creating thread\n"));
			return 3;
		}
	}
	WaitForMultipleObjects(nThreads, tHandles, TRUE, INFINITE);
	for (i = 0; i < nThreads; i++) {
		// want to be sure that it is not NULL
		assert(tHandles[i]);
		CloseHandle(tHandles[i]);
		collectData(threadIds[i]);
	}
	free(tHandles);
	return 0;
}

DWORD WINAPI visitDirectoryA(LPVOID param) {
	LPTSTR path = (LPTSTR)param;
	visitDirectoryRAndDo(path, 0, whatToDo, NULL);
	return 0;
}

VOID whatToDoA(LPTSTR path, LPTSTR entryName, HANDLE fHandle) {
	_tprintf(_T("%u - %s\n"), GetCurrentThreadId(), entryName);
	return;
}

VOID collectDataA(DWORD thId) {
	return;
}

DWORD WINAPI visitDirectoryB(LPVOID param) {
	LPTSTR path = (LPTSTR)param;
	HANDLE hOut;
	LPTSTR filename;

	filename = getFileNameByThreadId(GetCurrentThreadId());

	hOut = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Error creating file %s\n"), filename);
		free(filename);
		return 1;
	}
	free(filename);
	visitDirectoryRAndDo(path, 0, whatToDo, hOut);
	CloseHandle(hOut);
	return 0;
}

VOID whatToDoB(LPTSTR path, LPTSTR entryName, HANDLE fHandle) {
	DWORD strLen;
	DWORD nWrote;
	TCHAR line[MAX_PATH + 20];
	_stprintf(line, _T("%u - %s\n"), GetCurrentThreadId(), entryName);
	strLen = (DWORD)_tcscnlen(line, MAX_PATH + 20);
	assert(strLen < MAX_PATH + 20);
	if (!WriteFile(fHandle, line, strLen * sizeof(TCHAR), &nWrote, NULL) || nWrote != strLen * sizeof(TCHAR)) {
		_ftprintf(stderr, _T("%u - Error writing file"), GetCurrentThreadId());
	}
	return;
}

VOID collectDataB(DWORD thId) {
	LPTSTR filename;
	HANDLE fIn;
	LARGE_INTEGER fileSize;
	LPTSTR content;
	DWORD nRead;
	filename = getFileNameByThreadId(thId);
	fIn = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to open file %s\n"), filename);
		free(filename);
		return;
	}
	if (!GetFileSizeEx(fIn, &fileSize)) {
		_ftprintf(stderr, _T("Impossible to get file size for %s\n"), filename);
		free(filename);
		CloseHandle(fIn);
		return;
	}
	assert(fileSize.HighPart == 0);
	// allocate an extra position for the '\0' terminator
	content = (LPTSTR)malloc(fileSize.LowPart + sizeof(TCHAR));
	if (content == NULL) {
		_ftprintf(stderr, _T("Impossible to allocate content for file %s\n"), filename);
		free(filename);
		CloseHandle(fIn);
		return;
	}
	if (!ReadFile(fIn, content, fileSize.LowPart, &nRead, NULL) || nRead != fileSize.LowPart) {
		_ftprintf(stderr, _T("Error in reading file %s\n"), filename);
		free(filename);
		free(content);
		CloseHandle(fIn);
		return;
	}
	CloseHandle(fIn);
	content[fileSize.LowPart / sizeof(TCHAR)] = _T('\0');
	_tprintf(_T("%s"), content);
	free(filename);
	free(content);
	return;
}

DWORD WINAPI visitDirectoryC(LPVOID param) {
	LPTSTR path = (LPTSTR)param;
	EnterCriticalSection(&cs);
	visitDirectoryRAndDo(path, 0, whatToDo, NULL);
	LeaveCriticalSection(&cs);
	return 0;
}

VOID whatToDoC(LPTSTR path, LPTSTR entryName, HANDLE fHandle) {
	_tprintf(_T("%u - %s\n"), GetCurrentThreadId(), entryName);
	return;
}

VOID collectDataC(DWORD thId) {
	return;
}

VOID visitDirectoryRAndDo(LPTSTR path1, DWORD level, VOID(*toDo)(LPTSTR path, LPTSTR entryName, HANDLE fHandle), HANDLE fHandle) {
	WIN32_FIND_DATA findFileData;
	HANDLE hFind;
	TCHAR searchPath[MAX_PATH];
	TCHAR newPath1[MAX_PATH];

	// build the searchPath string, to be able to search inside path1: searchPath = path1\*
	_sntprintf(searchPath, MAX_PATH - 1, _T("%s\\*"), path1);
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
		toDo(findFileData.cFileName, newPath1, fHandle);
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
			visitDirectoryRAndDo(newPath1, level + 1, toDo, fHandle);
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

LPTSTR getFileNameByThreadId(DWORD tId) {
	LPTSTR result = (LPTSTR)malloc(MAX_PATH * sizeof(TCHAR));
	if (result == NULL) {
		return NULL;
	}
	_stprintf(result, _T("temp%u.txt"), tId);
	return result;
}