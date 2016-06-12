/*
Lab 12 exercise 03

Inter-process comunication using a temporary file.
The client creates a temporary file and launches the server process.
The clients writes on this file lines of a fixed size, while the
server reads them. The server at the end deletes the file.
Synchronization is done locking the lines of the file, and is required
because the server can read only when the client has finished writing
the line.

@Author: Martino Mensio
*/

#define DEBUG 0

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
// CTRL-Z code found on https://codereview.appspot.com/45150045
#define CTRL_Z 0x1A
#define END_MSG _T(".end")

VOID server(LPTSTR);
VOID client(LPTSTR);

BOOL lockFileLine(HANDLE hFile, DWORD line);
BOOL unlockFileLine(HANDLE hFile, DWORD line);

INT _tmain(INT argc, LPTSTR argv[]) {
	if (argc > 2 && _tcsncmp(argv[1], SERVER_PARAM, MAX_PATH) == 0) {
		server(argv[2]);
	} else if (argc == 1) {
		// the client is run only if no parameters are passed, in order to
		// prevent a "fork bomb" in case of a wrong parameter
		client(argv[0]);
	} else {
		_ftprintf(stderr, _T("Wrong command line parameters\n"));
	}
	return 0;
}

VOID server(LPTSTR tmpFileName) {
	HANDLE tmpFile;
	TCHAR line[MAX_LEN];
	DWORD nRead;
	DWORD i;
#if DEBUG
	_tprintf(_T("Server launched\n"));
#endif // DEBUG
	// open an existing file for read
	tmpFile = CreateFile(tmpFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (tmpFile == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create temporary file %s. Error: %x\n"), tmpFileName, GetLastError());
		return;
	}
	i = 0;
	while (TRUE) {
		// acquire the lock on the line (mutual exclusion)
		lockFileLine(tmpFile, i);
		// read the line
		if (!ReadFile(tmpFile, line, MAX_LEN * sizeof(TCHAR), &nRead, NULL)) {
			// should not happen
			_ftprintf(stderr, _T("Found file end. Error: %x\n"), GetLastError());
			unlockFileLine(tmpFile, i);
			break;
		}
		// always release locks!
		unlockFileLine(tmpFile, i);
		if (nRead != MAX_LEN * sizeof(TCHAR)) {
			// for example, nRead = 0 means that synchronization failed, and server tried to read after the end
			// should not happen
			_ftprintf(stderr, _T("Mismatch o the file record size %d\n"), nRead);
			break;
		}
		// print on the screen the line
		_tprintf(_T("server: %s"), line);
		// check if this was the end message
		if (_tcsncmp(line, END_MSG, MAX_LEN) == 0) {
			_tprintf(_T("Found the end message\n"));
			break;
		}
		i++;
	}
	CloseHandle(tmpFile);
	// delete the temporary file
	if (!DeleteFile(tmpFileName)) {
		_ftprintf(stderr, _T("Impossible to delete file. Error: %x\n"), GetLastError());
	}
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
	// get an unique temporary file name
	if (GetTempFileName(_T("."), _T("lab12es03"), 0, tmpFileName) == 0) {
		_ftprintf(stderr, _T("Impossible to create a temporary file name. Error: %x\n"), GetLastError());
		return;
	}
#if DEBUG
	_tprintf(_T("Temporary file name: %s\n"), tmpFileName);
#endif // DEBUG
	// create it (new file, for writing)
	tmpFile = CreateFile(tmpFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (tmpFile == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create temporary file %s. Error: %x\n"), tmpFileName, GetLastError());
		return;
	}
	// the executable file name (argv[0]) can have some spaces: sourround it with quotes
	_stprintf(executable, _T("\"%s\""), myName);
	// generate the argv for the server: executable --server tmpfile.tmp
	_stprintf(arguments, _T("%s %s %s"), executable, SERVER_PARAM, tmpFileName);
	// retrieve the startupInfo, which contains the stdin, stdout, stderr handles that will be inherited
	GetStartupInfo(&startupInfo);

#if DEBUG
	_tprintf(_T("Server will be launched with arguments: %s\n"), arguments);
#endif // DEBUG

	i = 0;
	// the first line is locked so that the server won't be able to proceed on the first read until the client releases it
	lockFileLine(tmpFile, i);
	// create the process by enabling the inheritance of handles
	if (!CreateProcess(myName, arguments, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo)) {
		_ftprintf(stderr, _T("Impossible to create server process. Error: %x\n"), GetLastError());
		return;
	}
	
	// set up the console in order to stop reading when user types CTRL-Z
	rcc.nLength = sizeof(CONSOLE_READCONSOLE_CONTROL);
	rcc.nInitialChars = 0;
	rcc.dwCtrlWakeupMask = 1 << CTRL_Z;
	rcc.dwControlKeyState = 0;

	while (TRUE) {
		_tprintf(_T("(CTRL-Z to stop reading) > "));
		// clean up the line buffer
		memset(line, 0, MAX_LEN * sizeof(TCHAR));
		// read a line from the console using the special structure for capturing CTRL-Z
		result = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), line, MAX_LEN - 3, &nRead, &rcc);
		line[MAX_LEN - 3] = _T('\r');
		line[MAX_LEN - 2] = _T('\n');
		line[MAX_LEN - 1] = _T('\0');
		lineLength = _tcsnlen(line, MAX_LEN);
		// look if last character read was CTRL-Z
		if (line[lineLength - 1] == CTRL_Z) {
#if DEBUG
			_tprintf(_T("Found CTRL-Z\n"));
#endif // DEBUG
			// write in the buffer the end mesage (if user typed some keys before, they won't be sent)
			_tcsncpy(line, END_MSG, MAX_LEN);
			break;
		}
		// write to temporary file the line
		if (!WriteFile(tmpFile, line, MAX_LEN * sizeof(TCHAR), &nWritten, NULL) || nWritten != MAX_LEN * sizeof(TCHAR)) {
			_ftprintf(stderr, _T("Impossible to write the string to temp file. Error: %x\n"), GetLastError());
			return;
		}
		// lock the next line
		lockFileLine(tmpFile, i + 1);
		// unlock the current line, so that the server will be able to read it
		unlockFileLine(tmpFile, i);
		i++;
	}
	// the last write (of the end message)
	if (!WriteFile(tmpFile, line, MAX_LEN * sizeof(TCHAR), &nWritten, NULL) || nWritten != MAX_LEN * sizeof(TCHAR)) {
		_ftprintf(stderr, _T("Impossible to write the string to temp file. Error: %x\n"), GetLastError());
		return;
	}
	// the last line lock is released automatically when file handle is closed
	CloseHandle(tmpFile);
#if DEBUG
	_tprintf(_T("Done. Now i wait server process\n"));
#endif // DEBUG
	
	WaitForSingleObject(processInfo.hProcess, INFINITE);
}

BOOL lockFileLine(HANDLE hFile, DWORD line) {
	OVERLAPPED ov = { 0, 0, 0, 0, NULL };
	LONGLONG n = line;
	LARGE_INTEGER filePos, size;
	DWORD nRead;
	filePos.QuadPart = n * MAX_LEN * sizeof(TCHAR);
	ov.Offset = filePos.LowPart;
	ov.OffsetHigh = filePos.HighPart;
	size.QuadPart = MAX_LEN * sizeof(TCHAR);
	// lock the portion of file. The OVERLAPPED is used to specify the starting displacement of the record to be locked
	// and its size is specified on the 4th and 5th parameter
	if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, size.LowPart, size.HighPart, &ov)) {
		_ftprintf(stderr, _T("Error locking file portion. Error: %x\n"), GetLastError());
		return FALSE;
	}
#if DEBUG
	_tprintf(_T("Aquired lock on line %d\n"), line);
#endif // DEBUG
	return TRUE;
}
BOOL unlockFileLine(HANDLE hFile, DWORD line) {
	OVERLAPPED ov = { 0, 0, 0, 0, NULL };
	LONGLONG n = line;
	LARGE_INTEGER filePos, size;
	DWORD nRead;
	filePos.QuadPart = n * MAX_LEN * sizeof(TCHAR);
	ov.Offset = filePos.LowPart;
	ov.OffsetHigh = filePos.HighPart;
	size.QuadPart = MAX_LEN * sizeof(TCHAR);
	// unlock the portion of file
	if (!UnlockFileEx(hFile, 0, size.LowPart, size.HighPart, &ov)) {
		_ftprintf(stderr, _T("Error unlocking file portion. Error: %x\n"), GetLastError());
		return FALSE;
	}
#if DEBUG
	_tprintf(_T("Released lock on line %d\n"), line);
#endif // DEBUG
	return TRUE;
}