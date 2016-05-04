#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE

#define STR_MAX_L 30+1	// extra 1 for \0

struct student {
	INT id;
	DWORD regNum;
	TCHAR surname[STR_MAX_L];
	TCHAR name[STR_MAX_L];
	INT mark;
};

INT _tmain(INT argc, LPTSTR argv[]) {

	HANDLE hOut;
	FILE *fInP;
	DWORD nOut;
	student s;

	if (argc != 3) {
		_ftprintf(stderr, _T("Usage: %s file_in file_out\n"), argv[0]);
		return 1;
	}

	fInP = _tfopen(argv[1], _T("rt"));

	if (fInP == NULL) {
		_ftprintf(stderr, _T("Cannot open input file %s. Error: %x\n"), argv[1], GetLastError());
		return 2;
	}

	hOut = CreateFile(argv[2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hOut == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"), argv[2], GetLastError());
		fclose(fInP);
		return 3;
	}

	while (_ftscanf(fInP, _T("%d %ld %s %s %d"), &s.id, &s.regNum, s.surname, s.name, &s.mark) == 5) {
		WriteFile(hOut, &s, sizeof(s), &nOut, NULL);
		if (nOut != sizeof(s)) {
			_ftprintf(stderr, _T("Cannot write correctly the output. Error: %x\n"), GetLastError());
			fclose(fInP);
			CloseHandle(hOut);
			return 4;
		}
	}
	_tprintf(_T("Correctly written the output file\n"));
	fclose(fInP);
	CloseHandle(hOut);

	_tprintf(_T("Now reading it back:\n"));
	HANDLE hIn;
	DWORD nIn;
	hIn = CreateFile(argv[2], GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open the file %s. Error: %x\n"), argv[2], GetLastError());
		return 5;
	}
	while (ReadFile(hIn, &s, sizeof(s), &nIn, NULL) && nIn > 0) {
		if (nIn != sizeof(s)) {
			_ftprintf(stderr, _T("Error reading, file shorter than expected\n"));
		}
		_tprintf(_T("id: %d\treg_number: %ld\tsurname: %10s\tname: %10s\tmark: %d\n"), s.id, s.regNum, s.surname, s.name, s.mark);
	}
	CloseHandle(hIn);
	return 0;
}