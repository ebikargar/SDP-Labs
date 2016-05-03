#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define BUF_SIZE 100

INT _tmain(INT argc, LPTSTR argv[])
{
	HANDLE hIn, hOut;
	DWORD nIn, nOut;
	CHAR buffer[BUF_SIZE];

	if (argc != 3) {
		fprintf(stderr, "Usage: cp file1 file2\n");
		return 1;
	}

	hIn = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
		return 2;
	}

	hOut = CreateFile(argv[2], GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open output file. Error: %x\n", GetLastError());
		CloseHandle(hIn);
		return 3;
	}

	while (ReadFile(hIn, buffer, BUF_SIZE, &nIn, NULL) && nIn > 0) {
		WriteFile(hOut, buffer, nIn, &nOut, NULL);
		if (nIn != nOut) {
			fprintf(stderr, "Fatal write error: %x\n", GetLastError());
			CloseHandle(hIn); CloseHandle(hOut);
			return 4;
		}
	}

	CloseHandle(hIn);
	CloseHandle(hOut);
	return 0;
}