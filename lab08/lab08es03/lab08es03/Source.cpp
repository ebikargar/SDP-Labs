#include <Windows.h>
#include <tchar.h>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE

//----------------------------------------
// change this value for switch the read/write called
// can be 'A', 'B' or 'C'
#define VERSION 'C'
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
#define STR_MAX_L 30+1	// one extra tchar for the string terminator

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

	// check number of parameters
	if (argc != 2) {
		_ftprintf(stderr, _T("Usage: %s input_file\n"), argv[0]);
		return 1;
	}

	// open the binary file for reading/writing
	hIn = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	// check the HANDLE value
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"), argv[2], GetLastError());
		return 3;
	}

	// main loop of the program
	while (1) {
		_tprintf(_T("Type a command : \"R id\" or \"W id\" or \"E\"\n> "));

		// read the command
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

// this function performs the random access by setting the file pointer
VOID readFP(INT id, HANDLE hIn) {
	student s;
	// the index should be 0-based because at the beginning of the file the student with id = 1 is stored
	LONGLONG n = id - 1;
	LARGE_INTEGER position;
	DWORD nRead;
	// the displacement in the file is obtained by multiplying the index by the size of a single element
	position.QuadPart = n * sizeof(s);
	// set the file pointer
	SetFilePointerEx(hIn, position, NULL, FILE_BEGIN);
	// and read
	if (ReadFile(hIn, &s, sizeof(s), &nRead, NULL) &&  nRead == sizeof(s)) {
		// read was successful
		_tprintf(_T("id: %d\treg_number: %ld\tsurname: %10s\tname: %10s\tmark: %d\n"), s.id, s.regNum, s.surname, s.name, s.mark);
	} else {
		// some errors in the read (out of bounds?)
		_ftprintf(stderr, _T("Error in read\n"));
	}
}

// this function performs the random access by setting the file pointer
VOID writeFP(INT id, HANDLE hIn) {
	student s;
	TCHAR command[CMD_MAX_LEN];
	// the index should be 0-based because at the beginning of the file the student with id = 1 is stored
	LONGLONG n = id - 1;
	LARGE_INTEGER position;
	DWORD nWritten;
	// the displacement in the file is obtained by multiplying the index by the size of a single element
	position.QuadPart = n * sizeof(s);
	SetFilePointerEx(hIn, position, NULL, FILE_BEGIN);
	_tprintf(_T("You required to write student with id = %d.\nInsert the following data: \"registration_number surname name mark\"\n> "), id);
	// the id was already entered by the user, no need to ask it again
	s.id = id;
	// read a line
	while (_fgetts(command, CMD_MAX_LEN, stdin)) {
		// parse the parameters
		if (_stscanf(command, _T("%ld %s %s %d"), &s.regNum, s.surname, s.name, &s.mark) != 4) {
			// if parse fails, ask again to input the data
			_tprintf(_T("Error in the string. The format is the following \"registration_number surname name mark\"\n> "));
		} else {
			// if parse is correct, can go on
			break;
		}
	}
	// write the record
	if (WriteFile(hIn, &s, sizeof(s), &nWritten, NULL) && nWritten == sizeof(s)) {
		// write was successful
		_tprintf(_T("Record with id = %d stored\n"), s.id);
	} else {
		// some errors in the write
		_ftprintf(stderr, _T("Error in write\n"));
	}
}

// this function performs the random acces by using an OVERLAPPED structure
VOID readOV(INT id, HANDLE hIn) {
	student s;
	DWORD nRead;
	// create a "clean" OVERLAPPED structure
	OVERLAPPED ov = { 0, 0, 0, 0, NULL };
	// the index should be 0-based because at the beginning of the file the student with id = 1 is stored
	LONGLONG n = id - 1;
	LARGE_INTEGER FilePos;
	// the displacement in the file is obtained by multiplying the index by the size of a single element
	FilePos.QuadPart = n * sizeof(s);
	// copy the displacement inside the OVERLAPPED structure
	ov.Offset = FilePos.LowPart;
	ov.OffsetHigh = FilePos.HighPart;
	_tprintf(_T("You required to read student with id = %d:\n"), id);
	// the read uses the OVERLAPPED
	if (ReadFile(hIn, &s, sizeof(s), &nRead, &ov) && nRead == sizeof(s)) {
		// read was successful
		_tprintf(_T("id: %d\treg_number: %ld\tsurname: %10s\tname: %10s\tmark: %d\n"), s.id, s.regNum, s.surname, s.name, s.mark);
	} else {
		// some errors in the read (out of bounds?)
		_ftprintf(stderr, _T("Error in read\n"));
	}
}

// this function performs the random acces by using an OVERLAPPED structure
VOID writeOV(INT id, HANDLE hIn) {
	student s;
	TCHAR command[CMD_MAX_LEN];
	DWORD nWritten;
	OVERLAPPED ov = { 0, 0, 0, 0, NULL };
	LONGLONG n = id - 1;
	LARGE_INTEGER FilePos;
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

// this function is similar to the readOV, but locks the record before reading it
VOID readLK(INT id, HANDLE hIn) {
	student s;
	OVERLAPPED ov = { 0, 0, 0, 0, NULL };
	LONGLONG n = id - 1;
	LARGE_INTEGER filePos, size;
	DWORD nRead;
	filePos.QuadPart = n * sizeof(s);
	ov.Offset = filePos.LowPart;
	ov.OffsetHigh = filePos.HighPart;
	size.QuadPart = sizeof(s);
	_tprintf(_T("You required to read student with id = %d:\n"), id);
	// lock the portion of file. The OVERLAPPED is used to specify the starting displacement of the record to be locked
	// and its size is specified on the 4th and 5th parameter
	if (!LockFileEx(hIn, LOCKFILE_EXCLUSIVE_LOCK, 0, size.LowPart, size.HighPart, &ov)) {
		_ftprintf(stderr, _T("Error locking file portion. Error: %x\n"), GetLastError());
		return;
	}
	// read the record
	if (ReadFile(hIn, &s, sizeof(s), &nRead, &ov) && nRead == sizeof(s)) {
		_tprintf(_T("id: %d\treg_number: %ld\tsurname: %10s\tname: %10s\tmark: %d\n"), s.id, s.regNum, s.surname, s.name, s.mark);
	} else {
		_ftprintf(stderr, _T("Error in read\n"));
	}
	// unlock the portion of file (arguments similar to the LockFileEx)
	if (!UnlockFileEx(hIn, 0, size.LowPart, size.HighPart, &ov)) {
		_ftprintf(stderr, _T("Error unlocking file portion. Error: %x\n"), GetLastError());
	}
}

VOID writeLK(INT id, HANDLE hIn) {
	student s;
	OVERLAPPED ov = { 0, 0, 0, 0, NULL };
	LONGLONG n = id - 1;
	LARGE_INTEGER filePos, size;
	DWORD nWritten;
	TCHAR command[CMD_MAX_LEN];
	filePos.QuadPart = n * sizeof(s);
	ov.Offset = filePos.LowPart;
	ov.OffsetHigh = filePos.HighPart;
	size.QuadPart = sizeof(s);
	_tprintf(_T("You required to write student with id = %d.\nInsert the following data: \"registration_number surname name mark\"\n> "), id);
	s.id = id;
	while (_fgetts(command, CMD_MAX_LEN, stdin)) {
		if (_stscanf(command, _T("%ld %s %s %d"), &s.regNum, s.surname, s.name, &s.mark) != 4) {
			_tprintf(_T("Error in the string. The format is the following \"registration_number surname name mark\"\n> "));
		} else {
			break;
		}

	}
	// lock the portion of file. The OVERLAPPED is used to specify the starting displacement of the record to be locked
	// and its size is specified on the 4th and 5th parameter
	if (!LockFileEx(hIn, LOCKFILE_EXCLUSIVE_LOCK, 0, size.LowPart, size.HighPart, &ov)) {
		_ftprintf(stderr, _T("Error locking file portion. Error: %x\n"), GetLastError());
		return;
	}
	// write the record
	if (WriteFile(hIn, &s, sizeof(s), &nWritten, &ov) && nWritten == sizeof(s)) {
		_tprintf(_T("Record with id = %d stored\n"), s.id);
	} else {
		_ftprintf(stderr, _T("Error in write\n"));
	}
	// unlock the portion of file
	if (!UnlockFileEx(hIn, 0, size.LowPart, size.HighPart, &ov)) {
		_ftprintf(stderr, _T("Error unlocking file portion. Error: %x\n"), GetLastError());
	}
}