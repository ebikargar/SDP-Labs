/*
Lab 11 exercise 02

TODO description

@Author: Martino Mensio
*/

// http://www.geeksforgeeks.org/factorial-large-number/

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


#define VERSION 'A'

#if VERSION == 'A'
#define SyncStruct SyncStructA
#define initializeSyncStruct initializeSyncStructA
#define producerWait producerWaitA
#define producerSignal producerSignalA
#define consumerWait consumerWaitA
#define consumerSignal consumerSignalA
#define DestroySyncStruct DestroySyncStructA
#endif // VERSION == 'A'
#if VERSION == 'B'
#define SyncStruct SyncStructB
#define initializeSyncStruct initializeSyncStructB
#define producerWait producerWaitB
#define producerSignal producerSignalB
#define consumerWait consumerWaitB
#define consumerSignal consumerSignalB
#define DestroySyncStruct DestroySyncStructB
#endif // VERSION == 'B'


// Maximum number of digits in output
#define MAXDIGITS 512

typedef struct _SyncStructA {
	HANDLE producerSem;			// producer waits 4 times on it, consumers do single signal on it
	HANDLE consumerSems[4];		// each consumer waits on its own, signalled by producer
} SyncStructA;

typedef struct _SyncStructB {
	HANDLE consumersEvent;		// consumers wait on it, MANUAL_RESET used with PulseEvent
	HANDLE producerEvents[4];	// producer waits on all of them, AUTO_RESET used with SetEvent (like binary semaphores)
} SyncStructB;

typedef SyncStruct* LPSyncStruct;

typedef struct _PARAM {
	INT threadNumber;
	LPLONG n;
	LPBOOL done;
	/*
	HANDLE producerSem;
	HANDLE consumerSem;
	*/
	LPSyncStruct sync;	// will be shared by all threads
} PARAM, *LPPARAM;

DWORD WINAPI consumer(LPVOID p);
LPTSTR factorial(LONG n);
INT multiply(INT x, INT res[], INT res_size);

BOOL initializeSyncStructA(LPSyncStruct);
BOOL producerWaitA(LPSyncStruct);
BOOL producerSignalA(LPSyncStruct);
BOOL consumerWaitA(LPSyncStruct, INT);
BOOL consumerSignalA(LPSyncStruct, INT);
BOOL DestroySyncStructA(LPSyncStruct);

BOOL initializeSyncStructB(LPSyncStruct);
BOOL producerWaitB(LPSyncStruct);
BOOL producerSignalB(LPSyncStruct);
BOOL consumerWaitB(LPSyncStruct, INT);
BOOL consumerSignalB(LPSyncStruct, INT);
BOOL DestroySyncStructB(LPSyncStruct);

INT _tmain(INT argc, LPTSTR argv[]) {
	HANDLE hIn;
	HANDLE consumerThreads[4];
	PARAM param[4];
	//HANDLE producerSem;
	INT i;
	LONG number;
	BOOL done;
	DWORD nRead;
	SyncStruct sync;

	// check number of parameters
	if (argc != 2) {
		_ftprintf(stderr, _T("Usage: %s input_file\n"), argv[0]);
		return 1;
	}

	hIn = CreateFile(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to open input file. Error: %x\n"), GetLastError());
		return 2;
	}

	if (!initializeSyncStruct(&sync)) {
		return 3;
	}
	/*
	producerSem = CreateSemaphore(NULL, 0, 4, NULL);
	if (producerSem == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create producer semaphore. Error: %x\n"), GetLastError());
		return 3;
	}*/

	for (i = 0; i < 4; i++) {
		/*
		param[i].consumerSem = CreateSemaphore(NULL, 0, 1, NULL);
		if (param[i].consumerSem == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create consumer semaphore. Error: %x\n"), GetLastError());
			return 4;
		}
		param[i].producerSem = producerSem;
		*/
		param[i].sync = &sync;
		param[i].threadNumber = i;
		param[i].n = &number;
		param[i].done = &done;
		consumerThreads[i] = CreateThread(NULL, 0, consumer, &param[i], 0, NULL);
	}

	done = FALSE;

	// start feeding the threads
	while (ReadFile(hIn, &number, sizeof(number), &nRead, NULL) && nRead > 0) {
		if (nRead != sizeof(number)) {
			_ftprintf(stderr, _T("Record size mismatch\n"));
			break;
		}
		producerSignal(&sync);
		/*
		for (i = 0; i < 4; i++) {
			ReleaseSemaphore(param[i].consumerSem, 1, NULL);
		}*/
		producerWait(&sync);
		/*
		for (i = 0; i < 4; i++) {
			WaitForSingleObject(producerSem, INFINITE);
		}*/
	}
	done = TRUE;
	producerSignal(&sync);
	/*
	for (i = 0; i < 4; i++) {
		ReleaseSemaphore(param[i].consumerSem, 1, NULL);
	}*/
	WaitForMultipleObjects(4, consumerThreads, TRUE, INFINITE);

	DestroySyncStruct(&sync);
	return 0;
}

DWORD WINAPI consumer(LPVOID p) {
	LPPARAM param = (LPPARAM)p;
	LONGLONG accumulator;
	LPTSTR resultNumber;
	LPTSTR stringResult;
	LONG i;
	accumulator = 0;
	if (param->threadNumber == 1) {
		accumulator = 1;
	}
	while(TRUE) {
		if (!consumerWait(param->sync, param->threadNumber)) {
			return 1;
		}
		//WaitForSingleObject(param->consumerSem, INFINITE);
		if (*param->done) {
			break;
		}
		// do the job
		//_tprintf(_T("Thread %d read value %ld\n"), param->threadNumber, *param->n);
		switch (param->threadNumber) {
		case 0:
			accumulator += *param->n;
			_tprintf(_T("Thread %d (partial sum) value: %lld\n"), param->threadNumber, accumulator);
			break;
		case 1:
			accumulator *= *param->n;
			_tprintf(_T("Thread %d (partial product) value: %lld\n"), param->threadNumber, accumulator);
			break;
		case 2:
			resultNumber = factorial(*param->n);
			_tprintf(_T("Thread %d (factorial) value: %s\n"), param->threadNumber, resultNumber);
			free(resultNumber);
			break;
		case 3:
			stringResult = (LPTSTR)calloc(*param->n, sizeof(TCHAR));
			for (i = 0; i < *param->n; i++) {
				stringResult[i] = _T('#');
			}
			stringResult[i] = 0;
			_tprintf(_T("%s\n"), stringResult);
			
			break;
		default:
			_ftprintf(stderr, _T("are you crazy?\n"));
			break;
		}
		if (!consumerSignal(param->sync, param->threadNumber)) {
			return 2;
		}
		//ReleaseSemaphore(param->producerSem, 1, NULL);
	}
	
	return 0;
}

BOOL initializeSyncStructA(LPSyncStruct sync) {
	INT i;
	sync->producerSem = CreateSemaphore(NULL, 0, 4, NULL);
	if (sync->producerSem == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create producer semaphore. Error: %x\n"), GetLastError());
		return FALSE;
	}
	for (i = 0; i < 4; i++) {
		sync->consumerSems[i] = CreateSemaphore(NULL, 0, 1, NULL);
		if (sync->consumerSems[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create consumer semaphore. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL producerWaitA(LPSyncStruct sync) {
	INT i;
	for (i = 0; i < 4; i++) {
		if (WaitForSingleObject(sync->producerSem, INFINITE) == WAIT_FAILED) {
			_ftprintf(stderr, _T("Wait on producer semaphore failed. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL producerSignalA(LPSyncStruct sync) {
	INT i;
	for (i = 0; i < 4; i++) {
		if (!ReleaseSemaphore(sync->consumerSems[i], 1, NULL)) {
			_ftprintf(stderr, _T("Error releasing consumer semaphore. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL consumerWaitA(LPSyncStruct sync, INT threadNumber) {
	if (WaitForSingleObject(sync->consumerSems[threadNumber], INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumer semaphore failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL consumerSignalA(LPSyncStruct sync, INT threadNumber) {
	if (!ReleaseSemaphore(sync->producerSem, 1, NULL)) {
		_ftprintf(stderr, _T("Error releasing producer semaphore. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL DestroySyncStructA(LPSyncStruct sync) {
	INT i;
	for (i = 0; i < 4; i++) {
		assert(sync->consumerSems[i]);
		CloseHandle(sync->consumerSems[i]);
	}
	assert(sync->producerSem);
	CloseHandle(sync->producerSem);
	return TRUE;
}

// This function finds factorial of large numbers and prints them
LPTSTR factorial(LONG n) {
	INT res[MAXDIGITS];
	LPTSTR result = (LPTSTR)calloc(MAXDIGITS, sizeof(TCHAR));

	// Initialize result
	res[0] = 1;
	INT res_size = 1;

	// Apply simple factorial formula n! = 1 * 2 * 3 * 4...*n
	for (INT x = 2; x <= n; x++)
		res_size = multiply(x, res, res_size);
	result[res_size] = 0;
	for (INT i = res_size - 1; i >= 0; i--) {
		result[res_size - 1 - i] = res[i] + _T('0');
	}

	return result;
}

// This function multiplies x with the number represented by res[].
// res_size is size of res[] or number of digits in the number represented
// by res[]. This function uses simple school mathematics for multiplication.
// This function may value of res_size and returns the new value of res_size
INT multiply(INT x, INT res[], INT res_size) {
	BOOL carry = 0;  // Initialize carry

					// One by one multiply n with individual digits of res[]
	for (INT i = 0; i<res_size; i++) {
		INT prod = res[i] * x + carry;
		res[i] = prod % 10;  // Store last digit of 'prod' in res[]
		carry = prod / 10;    // Put rest in carry
	}

	// Put carry in res and increase result size
	while (carry) {
		res[res_size] = carry % 10;
		carry = carry / 10;
		res_size++;
	}
	return res_size;
}
