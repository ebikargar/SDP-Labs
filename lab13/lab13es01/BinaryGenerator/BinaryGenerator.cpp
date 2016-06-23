/*
BinaryGenerator for lab 13 exercise 01

This program takes ASCII input files that contain for each row:
directoryName inputFileName outputName
The outputs are binary files in which the corresponding records are stored.

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

#define NAME_MAX_LEN 128
#define LINE_MAX_LEN 128*3 + 2 + 2 + 1 // two spaces, CR, LF, \0
#define EXTENSION_TO_APPEND _T(".bin")

typedef struct _record {
	TCHAR directoryName[NAME_MAX_LEN];
	TCHAR inputFileName[NAME_MAX_LEN];
	TCHAR outputName[NAME_MAX_LEN];
} RECORD;

typedef RECORD* LPRECORD;

VOID writeBinary(LPTSTR filename);

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

VOID writeBinary(LPTSTR filename) {
	HANDLE hOut;
	FILE *fInP;
	DWORD nOut;
	LPTSTR filename_bin;
	RECORD record;
	TCHAR line[LINE_MAX_LEN];

	// ope the text file for reading
	fInP = _tfopen(filename, _T("rt"));

	// check if open was successful
	if (fInP == NULL) {
		_ftprintf(stderr, _T("Cannot open input file %s. Error: %x\n"), filename, GetLastError());
		return;
	}

	// allocate a string to store the output file name
	filename_bin = (LPTSTR)malloc((_tcslen(filename) + _tcslen(EXTENSION_TO_APPEND) + 1) * sizeof(TCHAR));
	if (filename_bin == NULL) {
		_ftprintf(stderr, _T("Cannot allocate file name for file %s\n"), filename);
		fclose(fInP);
		return;
	}

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
		return;
	}

	// parse integers one by one
	while (_fgetts(line, LINE_MAX_LEN, fInP)) {
		line[LINE_MAX_LEN - 1] = 0;
		if (_stscanf(line, _T("%s %s %s"), &record.directoryName, &record.inputFileName, record.outputName) != 3) {
			_ftprintf(stderr, _T("Error reading a record! The format is not correct\n"));
			fclose(fInP);
			CloseHandle(hOut);
			free(filename_bin);
			return;
		}
		WriteFile(hOut, &record, sizeof(record), &nOut, NULL);
		if (nOut != sizeof(record)) {
			_ftprintf(stderr, _T("Cannot write correctly the output. Error: %x\n"), GetLastError());
			fclose(fInP);
			CloseHandle(hOut);
			free(filename_bin);
			return;
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
		return;
	}
	while (ReadFile(hIn, &record, sizeof(record), &nIn, NULL) && nIn > 0) {
		if (nIn != sizeof(record)) {
			_ftprintf(stderr, _T("Error reading, file shorter than expected\n"));
			CloseHandle(hIn);
			free(filename_bin);
			return;
		}
		_tprintf(_T("record: %s\t%s\t%s\n"), record.directoryName, record.inputFileName, record.outputName);
	}
	CloseHandle(hIn);
	free(filename_bin);
	return;
}