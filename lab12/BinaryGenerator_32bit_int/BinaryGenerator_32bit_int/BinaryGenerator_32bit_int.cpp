/*
Binary Generator

This program reads a variable number of text files (ASCII) containing
characters that represent unsigned integer values. For each of them,
it generates a corresponding file (with a name that is derived from the
corresponding input file by appending an extension) in which the parsed
integers are written in their binary representation (32 bits)

@Author: Martino Mensio
*/

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif // !UNICODE
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#define EXTENSION_TO_APPEND _T(".bin")

INT writeBinary(LPTSTR filename);

// the main receives a list of files to convert (at least 1)
INT _tmain(INT argc, LPTSTR argv[]) {

	if (argc < 2) {
		_ftprintf(stderr, _T("Usage: %s list_of_files\n"), argv[0]);
		return 1;
	}

	// for each parameter, do the conversion from ASCII to binary
	for (size_t i = 1; i < argc; i++) {
		writeBinary(argv[i]);
	}
	return 0;
}

INT writeBinary(LPTSTR filename) {
	HANDLE hOut;
	FILE *fInP;
	DWORD nOut;
	LONG num;
	LPTSTR filename_bin;

	// open the text file for reading
	fInP = _tfopen(filename, _T("rt"));

	// check if open was successful
	if (fInP == NULL) {
		_ftprintf(stderr, _T("Cannot open input file %s. Error: %x\n"), filename, GetLastError());
		return 2;
	}

	// allocate a string to store the output file name
	filename_bin = (LPTSTR)malloc((_tcslen(filename) + _tcslen(EXTENSION_TO_APPEND) + 1) * sizeof(TCHAR));

	// copy the source name
	_tcscpy(filename_bin, filename);
	// and append the extension
	_tcscat(filename_bin, EXTENSION_TO_APPEND);

	// open the binary file for writing
	hOut = CreateFile(filename_bin, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// check the handle value
	if (hOut == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Cannot open output file %s. Error: %x\n"), filename_bin, GetLastError());
		fclose(fInP);
		free(filename_bin);
		return 3;
	}

	// parse integers one by one
	while (_ftscanf(fInP, _T("%ld"), &num) == 1) {
		// and store them in the binary file
		WriteFile(hOut, &num, sizeof(num), &nOut, NULL);
		// check if write was successful
		if (nOut != sizeof(num)) {
			_ftprintf(stderr, _T("Cannot write correctly the output. Error: %x\n"), GetLastError());
			fclose(fInP);
			CloseHandle(hOut);
			free(filename_bin);
			return 4;
		}
	}
	_tprintf(_T("Correctly written the output file\n"));
	fclose(fInP);
	CloseHandle(hOut);

	// verification by reading back the binary file
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
			CloseHandle(hIn);
			free(filename_bin);
			return 6;
		}
		_tprintf(_T("Number: %ld\n"), num);
	}
	CloseHandle(hIn);
	free(filename_bin);
	return 0;
}
