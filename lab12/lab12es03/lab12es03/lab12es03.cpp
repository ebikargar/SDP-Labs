/*
Lab 12 exercise 03

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

#define SERVER_PARAM _T("--server")

VOID server(LPTSTR);
VOID client(LPTSTR);

INT _tmain(INT argc, LPTSTR argv[]) {
	/*
	_tprintf(_T("%d %s"), argc, argv[0]);
	if (argc > 1) {
		_tprintf(_T(" %s"), argv[1]);
		if (argc > 2) {
			_tprintf(_T(" %s"), argv[2]);
		}
	}
	_gettch();
	*/
	if (argc > 2 && _tcsncmp(argv[1], SERVER_PARAM, MAX_PATH) == 0) {
		server(argv[2]);
	} else {
		client(argv[0]);
	}
	
}

VOID server(LPTSTR tmpFileName) {
	_tprintf(_T("server\n"));
}
VOID client(LPTSTR myName) {
	HANDLE tmpFile = NULL;
	TCHAR tmpFileName[MAX_PATH];
	TCHAR arguments[MAX_PATH];
	TCHAR executable[MAX_PATH];
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
	if (GetTempFileName(_T("."), _T("lab12es03"), 0, tmpFileName) == 0) {
		_ftprintf(stderr, _T("Impossible to create a temporary file name. Error: %x\n"), GetLastError());
		return;
	}
	tmpFile = CreateFile(tmpFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if (tmpFile == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create temporary file %s. Error: %x\n"), tmpFileName, GetLastError());
		return;
	}
	_stprintf(executable, _T("\"%s\""), myName);
	//_stprintf(executable, _T("lab12es03.exe"));
	_stprintf(arguments, _T("%s %s %s"), executable, SERVER_PARAM, tmpFileName);
	GetStartupInfo(&startupInfo);

	if (!CreateProcess(myName, arguments, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo)) {
		_ftprintf(stderr, _T("Impossible to create server process. Error: %x\n"), GetLastError());
		return;
	}

	_tprintf(_T("Done\n"));
}