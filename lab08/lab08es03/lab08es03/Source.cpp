#include <Windows.h>
#include <tchar.h>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE

//----------------------------------------
// change this value for switch the read/write called
// can be 'A', 'B' or 'C'
#define VERSION 'B'
//----------------------------------------
#if VERSION == 'A'
#define read readFP
#define write writeFP
#endif // VERSION == 'A'
#if VERSION == 'B'
#define read readOV
#define write writeOV
#endif // VERSION == 'B'
#if VERSION == 'C'
#define read readLK
#define write writeLK
#endif // VERSION == 'C'

#define CMD_MAX_LEN 255
#define STR_MAX_L 30+1

struct student {
	INT id;
	DWORD regNum;
	TCHAR surname[STR_MAX_L];
	TCHAR name[STR_MAX_L];
	INT mark;
};

VOID readFP(INT id, HANDLE hIn);
VOID writeFP(INT id, HANDLE hIn);

VOID readOV(INT id, HANDLE hIn);
VOID writeOV(INT id, HANDLE hIn);

VOID readLK(INT id, HANDLE hIn);
VOID writeLK(INT id, HANDLE hIn);

INT _tmain(INT argc, LPTSTR argv[]) {

	TCHAR command[CMD_MAX_LEN];
	TCHAR op;
	INT id;
	HANDLE hIn;
	BOOL exit = FALSE;

	if (argc != 2) {
		_ftprintf(stderr, _T("Usage: %s input_file\n", argv[0]));
		return 1;
	}

	hIn = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"), argv[2], GetLastError());
		return 3;
	}

	while (1) {
		_tprintf(_T("Type a command : \"R id\" or \"W id\" or \"E\"\n> "));
		_fgetts(command, CMD_MAX_LEN, stdin);
		if (_stscanf(command, _T("%c"), &op) != 1) {
			_tprintf(_T("Command contains some errors\n"));
		} else {
			switch (op) {
			case 'R':
				if (_stscanf(command, _T("%*c %d"), &id) != 1) {
					_tprintf(_T("Command contains some errors\n"));
				} else {
					read(id, hIn);
				}
				break;
			case 'W':
				if (_stscanf(command, _T("%*c %d"), &id) != 1) {
					_tprintf(_T("Command contains some errors\n"));
				} else {
					write(id, hIn);
				}
				break;
			case 'E':
				exit = TRUE;
				break;
			default:
				_ftprintf(stderr, _T("Unknown operation %c\n"), op);
				break;
			}
		}
		if (exit) {
			break;
		}
	}
	CloseHandle(hIn);
	return 0;
}

VOID readFP(INT id, HANDLE hIn) {
	student s;
	LONGLONG n = id - 1;
	LARGE_INTEGER position;
	DWORD nRead;
	position.QuadPart = n * sizeof(s);
	SetFilePointerEx(hIn, position, NULL, FILE_BEGIN);
	if (ReadFile(hIn, &s, sizeof(s), &nRead, NULL)) {
		_tprintf(_T("id: %d\treg_number: %ld\tsurname: %10s\tname: %10s\tmark: %d\n"), s.id, s.regNum, s.surname, s.name, s.mark);
	} else {
		_ftprintf(stderr, _T("Error in read\n"));
	}
}

VOID writeFP(INT id, HANDLE hIn) {
	student s;
	TCHAR command[CMD_MAX_LEN];
	LONGLONG n = id - 1;
	LARGE_INTEGER position;
	DWORD nWritten;
	position.QuadPart = n * sizeof(s);
	SetFilePointerEx(hIn, position, NULL, FILE_BEGIN);
	_tprintf(_T("You required to write student with id = %d.\nInsert the following data: \"registration_number surname name mark\"\n> "), id);
	s.id = id;
	while (_fgetts(command, CMD_MAX_LEN, stdin)) {
		if (_stscanf(command, _T("%ld %s %s %d"), &s.regNum, s.surname, s.name, &s.mark) != 4) {
			_tprintf(_T("Error in the string. The format is the following \"registration_number surname name mark\"\n> "));
		} else {
			break;
		}

	}
	if (WriteFile(hIn, &s, sizeof(s), &nWritten, NULL) && nWritten == sizeof(s)) {
		_tprintf(_T("Record with id = %d stored\n"), s.id);
	} else {
		_ftprintf(stderr, _T("Error in write\n"));
	}
}

VOID readOV(INT id, HANDLE hIn) {
	student s;
	DWORD nRead;
	OVERLAPPED ov = { 0, 0, 0, 0, NULL };
	LONGLONG n = id - 1;
	LARGE_INTEGER FilePos;
	DWORD nRd, nWrt;
	FilePos.QuadPart = n * sizeof(s);
	ov.Offset = FilePos.LowPart;
	ov.OffsetHigh = FilePos.HighPart;
	_tprintf(_T("You required to read student with id = %d:\n"), id);
	if (ReadFile(hIn, &s, sizeof(s), &nRead, &ov) && nRead == sizeof(s)) {
		_tprintf(_T("id: %d\treg_number: %ld\tsurname: %10s\tname: %10s\tmark: %d\n"), s.id, s.regNum, s.surname, s.name, s.mark);
	} else {
		_ftprintf(stderr, _T("Error in read\n"));
	}
}

VOID writeOV(INT id, HANDLE hIn) {
	student s;
	TCHAR command[CMD_MAX_LEN];
	DWORD nWritten;
	OVERLAPPED ov = { 0, 0, 0, 0, NULL };
	LONGLONG n = id - 1;
	LARGE_INTEGER FilePos;
	DWORD nRd, nWrt;
	FilePos.QuadPart = n * sizeof(s);
	ov.Offset = FilePos.LowPart;
	ov.OffsetHigh = FilePos.HighPart;
	_tprintf(_T("You required to write student with id = %d.\nInsert the following data: \"registration_number surname name mark\"\n> "), id);
	s.id = id;
	while (_fgetts(command, CMD_MAX_LEN, stdin)) {
		if (_stscanf(command, _T("%ld %s %s %d"), &s.regNum, s.surname, s.name, &s.mark) != 4) {
			_tprintf(_T("Error in the string. The format is the following \"registration_number surname name mark\"\n> "));
		} else {
			break;
		}
		
	}
	if (WriteFile(hIn, &s, sizeof(s), &nWritten, &ov) && nWritten == sizeof(s)) {
		_tprintf(_T("Record with id = %d stored\n"), s.id);
	} else {
		_ftprintf(stderr, _T("Error in write\n"));
	}
}

VOID readLK(INT id, HANDLE hIn) {
	// TODO
}

VOID writeLK(INT id, HANDLE hIn) {
	// TODO
}