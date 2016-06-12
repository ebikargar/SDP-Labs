/*
Lab 12 exercise 02

Exercise about stuctured exception handling. The two threads can generate
 exceptions during their execution, that are captured in the __except block.
 Some exceptions are user-generated, while others are generated automatically.
 The other thread periodically checks a flag in the parameter in order to
 understand if it needs to terminate.

@Author: Martino Mensio
*/

// SAFE = TRUE -> the filter captures every exception
// SAFE = FALSE -> the filter only captures some exceptions
#define SAFE TRUE

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

// user-defined codes for some exceptions (not automatically thrown)
#define FILE_HANDLE_EXCEPTION 1
#define ODD_VALUES_EXCEPTION 2
#define MEMORY_ALLOCATION_EXCEPTION 3
#define UNEXPECTED_END_OF_FILE_EXCEPTION 4


#define MAX_LENGTH_RESULT_STR 2048

typedef struct _PARAM {
	CRITICAL_SECTION cs;	// protects the terminate flag
	volatile BOOL terminate; // if TRUE, the thread must terminate
	LPTSTR fileName;		// name of the file to process
	DWORD threadIds[2];		// not used, but could be used to terminate the other thread (dangerous)
} PARAM, *LPPARAM;

DWORD WINAPI firstThread(LPVOID);
DWORD WINAPI secondThread(LPVOID);

VOID ReportException(LPCTSTR UserMessage, DWORD ExceptionCode);
DWORD myFirstFilter(DWORD code);
DWORD mySecondFilter(DWORD code);

INT compareLong(LPCVOID a, LPCVOID b);
BOOL checkTerminate(LPPARAM param);

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
	__try {
		HANDLE hIn = CreateFile(param->fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hIn == INVALID_HANDLE_VALUE) {
			ReportException(_T("First thread: impossible to open file"), FILE_HANDLE_EXCEPTION);
		}
		__try {
			while (ReadFile(hIn, number, 2 * sizeof(LONG), &nRead, NULL) && nRead > 0) {
				// check if the values have been read
				if (nRead != 2 * sizeof(LONG)) {
					ReportException(_T("First thread: read size mismatch"), ODD_VALUES_EXCEPTION);
				}
				// the division can cause a EXCEPTION_INT_DIVIDE_BY_ZERO
				_tprintf(_T("First thread result: %ld / %ld = %ld\n"), number[0], number[1], number[0] / number[1]);
				// check if the other thread is ok
				if (checkTerminate(param)) {
					_tprintf(_T("They told me to terminate\n"));
					__leave;
				}
			}
		}
		__finally {
			CloseHandle(hIn);
			//_tprintf(_T("finally\n"));
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
	LPLONG numbers = NULL;
	LONG nOfEntries;
	DWORD nRead;
	LONG i;
	TCHAR result[MAX_LENGTH_RESULT_STR];
	__try {
		HANDLE hIn = CreateFile(param->fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hIn == INVALID_HANDLE_VALUE) {
			ReportException(_T("Second thread: impossible to open file"), FILE_HANDLE_EXCEPTION);
		}
		__try {
			if (!ReadFile(hIn, &nOfEntries, sizeof(LONG), &nRead, NULL) || nRead != sizeof(LONG)) {
				ReportException(_T("Second Thread: impossible to read the number of entries"), UNEXPECTED_END_OF_FILE_EXCEPTION);
			}
			numbers = (LPLONG)malloc(nOfEntries * sizeof(LONG));
			// I wanted to use a dedicated heap with HEAP_GENERATE_EXCEPTIONS, but
			// HeapAlloc generates exceptions only on second call even though there is not enough memory for the first call
			if (numbers == NULL) {
				ReportException(_T("Second Thread: impossible to allocate the array"), MEMORY_ALLOCATION_EXCEPTION);
			}

			i = 0;
			while (ReadFile(hIn, &numbers[i], sizeof(LONG), &nRead, NULL) && nRead > 0) {
				i++;
				if (nRead != sizeof(LONG)) {
					ReportException(_T("Second thread: read size mismatch"), UNEXPECTED_END_OF_FILE_EXCEPTION);
				}
				if (checkTerminate(param)) {
					// go to finally
					_tprintf(_T("They told me to terminate\n"));
					__leave;
				}
			}
			// i can't be greater than nOfEntries, because an exception on the numbers array would have been thrown
			if (i != nOfEntries) {
				ReportException(_T("Second thread: number of entries mismatch"), UNEXPECTED_END_OF_FILE_EXCEPTION);
			}
			//_tprintf(_T("Second thread successfully read\n"));

			// sort array
			qsort(numbers, nOfEntries, sizeof(LONG), compareLong);
			_tcsncpy(result, _T("Second thread sorted:"), MAX_LENGTH_RESULT_STR);
			for (i = 0; i < nOfEntries; i++) {
				_stprintf(result, _T("%s %ld"), result, numbers[i]);
			}
			_tprintf(_T("%s\n"), result);
		}
		__finally {
			CloseHandle(hIn);
			if (numbers != NULL) {
				free(numbers);
			}
			//_tprintf(_T("finally\n"));
		}
	}
	__except (mySecondFilter(GetExceptionCode())) {
		EnterCriticalSection(&param->cs);
		param->terminate = TRUE;
		LeaveCriticalSection(&param->cs);
	}
	return 0;
}

// copied from the slides
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
		// it's an user-generated exception
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
		// it's an automatically-generated exception
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
#if SAFE == TRUE
	if (result == EXCEPTION_CONTINUE_SEARCH) {
		result = EXCEPTION_EXECUTE_HANDLER;
		_tprintf(_T("Saved at the last chance. Catched an unpredicted exception, be careful: %x\n"), code);
	}
#endif // SAFE
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
			break;
		case UNEXPECTED_END_OF_FILE_EXCEPTION:
			_tprintf(_T("catched a UNEXPECTED_END_OF_FILE_EXCEPTION\n"));
			break;
		case MEMORY_ALLOCATION_EXCEPTION:
			_tprintf(_T("catched a MEMORY_ALLOCATION_EXCEPTION\n"));
			break;
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
#if SAFE == TRUE
	if (result == EXCEPTION_CONTINUE_SEARCH) {
		result = EXCEPTION_EXECUTE_HANDLER;
		_tprintf(_T("Saved at the last chance. Catched an unpredicted exception, be careful: %x\n"), code);
	}
#endif // SAFE
	return result;
}

// used in qsort
INT compareLong(LPCVOID a, LPCVOID b) {
	LONG la = *((LPLONG)a);
	LONG lb = *((LPLONG)b);
	LONG diff = la - lb;
	return (diff > 0) ? 1 : (diff == 0) ? 0 : -1;
}

BOOL checkTerminate(LPPARAM param) {
	BOOL terminate;
	EnterCriticalSection(&param->cs);
	terminate = param->terminate;
	LeaveCriticalSection(&param->cs);
	return terminate;
}