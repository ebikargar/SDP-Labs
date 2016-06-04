/*
Lab 12 exercise 02

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

#define FILE_HANDLE_EXCEPTION 1
#define ODD_VALUES_EXCEPTION 2
//#define FILE_WRONG_FORMAT_EXCEPTION 3
#define HEAP_CREATE_FAILED_EXCEPTION 3
#define UNEXPECTED_END_OF_FILE_EXCEPTION 4

typedef struct _PARAM {
	CRITICAL_SECTION cs;
	volatile BOOL terminate;
	LPTSTR fileName;
	DWORD threadIds[2];
} PARAM, *LPPARAM;

DWORD WINAPI firstThread(LPVOID);
DWORD WINAPI secondThread(LPVOID);

VOID ReportException(LPCTSTR UserMessage, DWORD ExceptionCode);
DWORD myFirstFilter(DWORD code);
DWORD mySecondFilter(DWORD code);

INT _tmain(INT argc, LPTSTR argv[]) {
	HANDLE hThreads[2];
	PARAM param;

	// check number of parameters
	if (argc != 2) {
		_ftprintf(stderr, _T("Usage: %s input_file\n"), argv[0]);
		return 1;
	}

	param.fileName = argv[1];
	param.terminate = FALSE;
	InitializeCriticalSection(&param.cs);
	
	hThreads[0] = CreateThread(NULL, 0, firstThread, &param, CREATE_SUSPENDED, &param.threadIds[0]);
	if (hThreads[0] == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create first thread. Error: %x\n"), GetLastError());
		return 2;
	}
	__try {
		hThreads[1] = CreateThread(NULL, 0, secondThread, &param, CREATE_SUSPENDED, &param.threadIds[1]);
		if (hThreads[1] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create second thread. Error: %x\n"), GetLastError());
			return 3;
		}
		__try {
			ResumeThread(hThreads[0]);
			ResumeThread(hThreads[1]);
			if (WaitForMultipleObjects(2, hThreads, TRUE, INFINITE) == WAIT_FAILED) {
				_ftprintf(stderr, _T("Wait failed. Error: %x\n"), GetLastError());
				return 4;
			}
		}
		__finally {
			CloseHandle(hThreads[1]);
		}
	}
	__finally {
		CloseHandle(hThreads[0]);
	}

	return 0;
}

DWORD WINAPI firstThread(LPVOID p) {
	LPPARAM param = (LPPARAM)p;
	LONG number[2];
	DWORD nRead;
	BOOL terminate;
	__try {
		HANDLE hIn = CreateFile(param->fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hIn == INVALID_HANDLE_VALUE) {
			ReportException(_T("First thread: impossible to open file"), FILE_HANDLE_EXCEPTION);
		}
		__try {
			// maybe enable exceptions for arithmetic operations?
			while (ReadFile(hIn, number, 2 * sizeof(LONG), &nRead, NULL) && nRead > 0) {
				if (nRead != 2 * sizeof(LONG)) {
					ReportException(_T("First thread: read size mismatch"), ODD_VALUES_EXCEPTION);
				}
				_tprintf(_T("First thread result: %ld\n"), number[0] / number[1]);
				EnterCriticalSection(&param->cs);
				terminate = param->terminate;
				LeaveCriticalSection(&param->cs);
				if (terminate) {
					_tprintf(_T("They told me to terminate\n"));
					__leave;
				}
			}
		}
		__finally {
			CloseHandle(hIn);
			_tprintf(_T("finally\n"));
		}
	}
	__except (myFirstFilter(GetExceptionCode())) {
		EnterCriticalSection(&param->cs);
		param->terminate = TRUE;
		LeaveCriticalSection(&param->cs);
	}
	return 0;
}
DWORD WINAPI secondThread(LPVOID p) {
	LPPARAM param = (LPPARAM)p;
	LPLONG numbers;
	LONG nOfEntries;
	DWORD nRead;
	BOOL terminate;
	HANDLE heap;
	LONG i;
	__try {
		HANDLE hIn = CreateFile(param->fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hIn == INVALID_HANDLE_VALUE) {
			ReportException(_T("Second thread: impossible to open file"), FILE_HANDLE_EXCEPTION);
		}
		__try {
			if (!ReadFile(hIn, &nOfEntries, sizeof(LONG), &nRead, NULL) || nRead != sizeof(LONG)) {
				ReportException(_T("Second Thread: impossible to read the number of entries"), UNEXPECTED_END_OF_FILE_EXCEPTION);
			}
			heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0, 24);
			if (heap == NULL) {
				ReportException(_T("Second thread: impossible to create a new heap"), HEAP_CREATE_FAILED_EXCEPTION);
			}
			numbers = (LPLONG)HeapAlloc(heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, nOfEntries * sizeof(LONG));
			// HeapAlloc generates exceptions only on second call??
			//numbers = (LPLONG)HeapAlloc(heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, nOfEntries * sizeof(LONG));
			i = 0;
			while (ReadFile(hIn, &numbers[i], sizeof(LONG), &nRead, NULL) && nRead > 0) {
				i++;
				if (nRead != sizeof(LONG)) {
					ReportException(_T("Second thread: read size mismatch"), UNEXPECTED_END_OF_FILE_EXCEPTION);
				}
				EnterCriticalSection(&param->cs);
				terminate = param->terminate;
				LeaveCriticalSection(&param->cs);
				if (terminate) {
					// go to finally
					_tprintf(_T("They told me to terminate\n"));
					__leave;
				}
			}
			// i can't be greater than nOfEntries, because an exception on the numbers array would have been thrown
			if (i != nOfEntries) {
				ReportException(_T("Second thread: number of entries mismatch"), UNEXPECTED_END_OF_FILE_EXCEPTION);
			}
			_tprintf(_T("Second thread successfully read\n"));

			// TODO sort array
			// this is a EXCEPTION_ACCESS_VIOLATION
			numbers[12345] = 0;
			// TODO print array

			HeapFree(heap, HEAP_GENERATE_EXCEPTIONS, numbers);
		}
		__finally {
			CloseHandle(hIn);
			_tprintf(_T("finally\n"));
		}
	}
	__except (mySecondFilter(GetExceptionCode())) {
		EnterCriticalSection(&param->cs);
		param->terminate = TRUE;
		LeaveCriticalSection(&param->cs);
	}
	return 0;
}

VOID ReportException(LPCTSTR UserMessage, DWORD ExceptionCode) {
	if (lstrlen(UserMessage) > 0) {
		_ftprintf(stderr, _T("%s\n"), UserMessage);
	}
	if (ExceptionCode != 0) {
		RaiseException((0x0FFFFFFF & ExceptionCode) | 0xE0000000, 0, 0, NULL);
	}
	return;
}

DWORD myFirstFilter(DWORD code) {
	DWORD result = EXCEPTION_EXECUTE_HANDLER;
	DWORD userCode;
	if ((0x20000000 & code)) {
		userCode = (0x0FFFFFFF & code);
		switch (userCode) {
		case FILE_HANDLE_EXCEPTION:
			_tprintf(_T("catched a FILE_HANDLE_EXCEPTION\n"));
			break;
		case ODD_VALUES_EXCEPTION:
			_tprintf(_T("catched a ODD_VALUES_EXCEPTION\n"));
			break;
		default:
			result = EXCEPTION_CONTINUE_SEARCH;
			break;
		}
	} else {
		switch (code) {
		case EXCEPTION_INVALID_HANDLE:
			// never catched
			_tprintf(_T("catched a EXCEPTION_INVALID_HANDLE\n"));
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			_tprintf(_T("catched a DIIDE_BY_ZERO exception\n"));
			break;
		default:
			result = EXCEPTION_CONTINUE_SEARCH;
			break;
		}
	}
	if (result == EXCEPTION_CONTINUE_SEARCH) {
		result = EXCEPTION_EXECUTE_HANDLER;
		_tprintf(_T("Saved at the last chance. Catched an unpredicted exception, be careful: %x\n"), code);
	}
	return result;
}
DWORD mySecondFilter(DWORD code) {
	DWORD result = EXCEPTION_EXECUTE_HANDLER;
	DWORD userCode;
	if ((0x20000000 & code)) {
		userCode = (0x0FFFFFFF & code);
		switch (userCode) {
		case FILE_HANDLE_EXCEPTION:
			_tprintf(_T("catched a FILE_HANDLE_EXCEPTION\n"));
		case UNEXPECTED_END_OF_FILE_EXCEPTION:
			_tprintf(_T("catched a UNEXPECTED_END_OF_FILE_EXCEPTION\n"));
		case HEAP_CREATE_FAILED_EXCEPTION:
			_tprintf(_T("catched a HEAP_CREATE_FAILED_EXCEPTION\n"));
		default:
			result = EXCEPTION_CONTINUE_SEARCH;
			break;
		}
	} else {
		switch (code) {
		case STATUS_NO_MEMORY:
			// never catched
			_tprintf(_T("catched a STATUS_NO_MEMORY\n"));
			break;
		case EXCEPTION_ACCESS_VIOLATION:
			_tprintf(_T("catched a EXCEPTION_ACCESS_VIOLATION\n"));
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			_tprintf(_T("catched a EXCEPTION_ARRAY_BOUNDS_EXCEEDED\n"));
		default:
			result = EXCEPTION_CONTINUE_SEARCH;
			break;
		}
	}
	if (result == EXCEPTION_CONTINUE_SEARCH) {
		_tprintf(_T("Saved at the last chance. Catched an unpredicted exception, be careful: %x\n"), code);
	}
	return result;
}