#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#define EXTENSION_TO_APPEND _T(".bin")

INT writeBinary(LPTSTR filename);

INT _tmain(INT argc, LPTSTR argv[]) {

	if (argc <2) {
		_ftprintf(stderr, _T("Usage: %s file_in file_out\n"), argv[0]);
		return 1;
	}

	for (size_t i = 1; i < argc; i++) {
		writeBinary(argv[i]);
	}
	return 0;
}

INT writeBinary(LPTSTR filename) {
	HANDLE hOut;
	FILE *fInP;
	DWORD nOut;
	UINT num;

	LPTSTR filename_bin = (LPTSTR) malloc(_tcslen(filename) + _tcslen(EXTENSION_TO_APPEND) + 1);

	_tcscpy(filename_bin, filename);
	_tcscat(filename_bin, EXTENSION_TO_APPEND);

	fInP = _tfopen(filename, _T("rt"));

	if (fInP == NULL) {
		_ftprintf(stderr, _T("Cannot open input file %s. Error: %x\n"), filename, GetLastError());
		return 2;
	}

	hOut = CreateFile(filename_bin, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hOut == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"), filename_bin, GetLastError());
		fclose(fInP);
		return 3;
	}

	while (_ftscanf(fInP, _T("%u"), &num) == 1) {
		WriteFile(hOut, &num, sizeof(num), &nOut, NULL);
		if (nOut != sizeof(num)) {
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
	hIn = CreateFile(filename_bin, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open the file %s. Error: %x\n"), filename_bin, GetLastError());
		return 5;
	}
	while (ReadFile(hIn, &num, sizeof(num), &nIn, NULL) && nIn > 0) {
		if (nIn != sizeof(num)) {
			_ftprintf(stderr, _T("Error reading, file shorter than expected\n"));
		}
		_tprintf(_T("Number: %u\n"), num);
	}
	CloseHandle(hIn);
	return 0;
}