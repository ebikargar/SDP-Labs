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
#define MAX_LEN 100
#define CTRL_Z 0x1A
#define END_MSG _T(".end")

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
	HANDLE tmpFile;
	TCHAR line[MAX_LEN];
	DWORD nRead;
	
	tmpFile = CreateFile(tmpFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (tmpFile == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create temporary file %s. Error: %x\n"), tmpFileName, GetLastError());
		return;
	}

	while (ReadFile(tmpFile, line, MAX_LEN * sizeof(TCHAR), &nRead, NULL)) {
		if (nRead != MAX_LEN * sizeof(TCHAR)) {
			_ftprintf(stderr, _T("Mismatch o the file record size\n"));
			break;
		}
		_tprintf(_T("server: %s"), line);
	}

	DeleteFile(tmpFileName);
	_tprintf(_T("server\n"));
}
VOID client(LPTSTR myName) {
	HANDLE tmpFile = NULL;
	TCHAR tmpFileName[MAX_PATH];
	TCHAR arguments[MAX_PATH];
	TCHAR executable[MAX_PATH];
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
	TCHAR line[MAX_LEN];
	DWORD lineLength;
	DWORD nWritten, nRead;
	DWORD i;
	CONSOLE_READCONSOLE_CONTROL rcc;
	BOOL result;
	if (GetTempFileName(_T("."), _T("lab12es03"), 0, tmpFileName) == 0) {
		_ftprintf(stderr, _T("Impossible to create a temporary file name. Error: %x\n"), GetLastError());
		return;
	}
	tmpFile = CreateFile(tmpFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
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
	i = 0;
	rcc.nLength = sizeof(CONSOLE_READCONSOLE_CONTROL);
	rcc.nInitialChars = 0;
	rcc.dwCtrlWakeupMask = 1 << CTRL_Z;
	rcc.dwControlKeyState = 0;
	while (TRUE) {
		_tprintf(_T("(CTRL-Z to stop reading) > "));
		memset(line, 0, MAX_LEN * sizeof(TCHAR));
		//_fgetts(line, MAX_LEN, stdin);
		result = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), line, MAX_LEN, &nRead, &rcc);
		lineLength = _tcsnlen(line, MAX_LEN);
		// look if last character read was CTRL-Z
		if (line[lineLength - 1] == CTRL_Z) {
			_tprintf(_T("Found CTRL-Z\n"));
			_tcsncpy(line, END_MSG, MAX_LEN);
			break;
		}
		// TODOS:
		// lock new record
		// unlock previous record
		if (!WriteFile(tmpFile, line, MAX_LEN * sizeof(TCHAR), &nWritten, NULL) || nWritten != MAX_LEN * sizeof(TCHAR)) {
			_ftprintf(stderr, _T("Impossible to write the string to temp file. Error: %x\n"), GetLastError());
			return;
		}
		i++;
	}
	if (!WriteFile(tmpFile, line, MAX_LEN * sizeof(TCHAR), &nWritten, NULL) || nWritten != MAX_LEN * sizeof(TCHAR)) {
		_ftprintf(stderr, _T("Impossible to write the string to temp file. Error: %x\n"), GetLastError());
		return;
	}

	_tprintf(_T("Done\n"));
}