/*
Lab 11 exercise 02

TODO description

@Author: Martino Mensio
*/

// change there for choosing the version
#define VERSION 'C'

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

#if VERSION == 'A'
#define SyncStruct SyncStructA
#endif // VERSION == 'A'
#if VERSION == 'B'
#define SyncStruct SyncStructB
#endif // VERSION == 'B'
#if VERSION == 'C'
#define SyncStruct SyncStructC
#endif // VERSION == 'C'


// Maximum number of digits in output
#define MAXDIGITS 512

typedef struct _SyncStructA {
	HANDLE producerSem;			// producer waits 4 times on it, consumers do single signal on it
	HANDLE consumerSems[4];		// each consumer waits on its own, signalled by producer
} SyncStructA;

typedef struct _SyncStructB {
	HANDLE consumerEvents[4];
	HANDLE producerEvents[4];	// producer waits on all of them, AUTO_RESET used with SetEvent (like binary semaphores)
	// maybe use semaphores at this point?
	/*
		Cannot use a single event that every customer waits on it, because MANUAL_RESET events used with PulseEvent
		are not reliable. When calling the PulseEvent, only the thread that are currently waiting for the event can
		go on, but there is no way of being sure that every consumer is already waiting. If some consumers are not
		yet waiting on the event, it won't be able to go on when it starts waiting, also because the producer will
		be waiting for it on his own event/semaphore.
		Doing it with a MANUAL_RESET together with a SetEvent is risky because some fast consumer can iterate more
		times while the event is signalled. To fix that, it is possible to use a 2-barriers mechanism in which the
		last thread of each barrier resets the event (see version C).
	*/
} SyncStructB;


/*
	consumers flow:
	+-----------------------+---------------------------------+-------------+
	                        |                                 |
	    prelude room     door 1     room 1 (work there)     door 2     out
	                        |                                 |
	+-----------------------+---------------------------------+-------------+

	Door 1 is opened by the producer, to allow consumer to enter room 1 and do their job with the data.
	Door 1 is closed by the last producer passing through it.
	Door 2 is opened by the last producer entering room 1 (the one that has already closed door 1).
	Door 2 is closed by the last producer going out.
*/
typedef struct _SyncStructC {
	CRITICAL_SECTION cs;		// to protect the number of threads in rooms
	HANDLE consumersFirstDoor;	// MANUAL_RESET
	HANDLE consumersSecondDoor; // MANUAL_RESET
	HANDLE producerDoor;		// AUTO_RESET
	volatile INT threadInFirstRoom;
	volatile INT threadInSecondRoom;
} SyncStructC;

typedef SyncStruct* LPSyncStruct;

typedef struct _PARAM {
	INT threadNumber;
	LPLONG n;
	LPBOOL done;
	LPSyncStruct sync;	// will be shared by all threads
} PARAM, *LPPARAM;

DWORD WINAPI consumer(LPVOID p);
LPTSTR factorial(LONG n);
INT multiply(INT x, INT res[], INT res_size);

BOOL initializeSyncStruct(LPSyncStruct);
BOOL producerWait(LPSyncStruct);
BOOL producerSignal(LPSyncStruct);
BOOL consumerWait(LPSyncStruct, INT);
BOOL consumerSignal(LPSyncStruct, INT);
BOOL DestroySyncStruct(LPSyncStruct);

INT _tmain(INT argc, LPTSTR argv[]) {
	HANDLE hIn;
	HANDLE consumerThreads[4];
	PARAM param[4];
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

	for (i = 0; i < 4; i++) {
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
		if (!producerSignal(&sync)) {
			return 4;
		}
		if (!producerWait(&sync)) {
			return 5;
		}
	}
	done = TRUE;
	if (!producerSignal(&sync)) {
		return 6;
	}
	
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
	}
	
	return 0;
}



#if VERSION == 'A'
BOOL initializeSyncStruct(LPSyncStruct sync) {
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
BOOL producerWait(LPSyncStruct sync) {
	INT i;
	for (i = 0; i < 4; i++) {
		if (WaitForSingleObject(sync->producerSem, INFINITE) == WAIT_FAILED) {
			_ftprintf(stderr, _T("Wait on producer semaphore failed. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL producerSignal(LPSyncStruct sync) {
	INT i;
	for (i = 0; i < 4; i++) {
		if (!ReleaseSemaphore(sync->consumerSems[i], 1, NULL)) {
			_ftprintf(stderr, _T("Error releasing consumer semaphore. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL consumerWait(LPSyncStruct sync, INT threadNumber) {
	if (WaitForSingleObject(sync->consumerSems[threadNumber], INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumer semaphore failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL consumerSignal(LPSyncStruct sync, INT threadNumber) {
	if (!ReleaseSemaphore(sync->producerSem, 1, NULL)) {
		_ftprintf(stderr, _T("Error releasing producer semaphore. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL DestroySyncStruct(LPSyncStruct sync) {
	INT i;
	for (i = 0; i < 4; i++) {
		assert(sync->consumerSems[i]);
		CloseHandle(sync->consumerSems[i]);
	}
	assert(sync->producerSem);
	CloseHandle(sync->producerSem);
	return TRUE;
}
#endif // VERSION == 'A'



#if VERSION == 'B'
BOOL initializeSyncStruct(LPSyncStruct sync) {
	INT i;
	/*
	sync->consumersEvent = CreateEvent(NULL, TRUE, TRUE, NULL);	// TODO probably the initial state sucks
	if (sync->consumersEvent == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create producer semaphore. Error: %x\n"), GetLastError());
		return FALSE;
	}
	*/
	for (i = 0; i < 4; i++) {
		sync->consumerEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);	// TODO probably the initial state sucks
		if (sync->consumerEvents[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create producer semaphore. Error: %x\n"), GetLastError());
			return FALSE;
		}
		sync->producerEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (sync->producerEvents[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create consumer semaphore. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL producerWait(LPSyncStruct sync) {
	if (WaitForMultipleObjects(4, sync->producerEvents, TRUE, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on producer events failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL producerSignal(LPSyncStruct sync) {
	INT i;
	/*
	if (!PulseEvent(sync->consumersEvent)) {
		_ftprintf(stderr, _T("Error pulsing consumer event. Error: %x\n"), GetLastError());
		return FALSE;
	}
	*/
	for (i = 0; i < 4; i++) {
		if (!SetEvent(sync->consumerEvents[i])) {
			_ftprintf(stderr, _T("Error setting consumer event. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL consumerWait(LPSyncStruct sync, INT threadNumber) {
	if (WaitForSingleObject(sync->consumerEvents[threadNumber], INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumer event failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL consumerSignal(LPSyncStruct sync, INT threadNumber) {
	if (!SetEvent(sync->producerEvents[threadNumber])) {
		_ftprintf(stderr, _T("Error setting producer event. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL DestroySyncStruct(LPSyncStruct sync) {
	INT i;
	for (i = 0; i < 4; i++) {
		assert(sync->producerEvents[i]);
		CloseHandle(sync->producerEvents[i]);
		assert(sync->consumerEvents[i]);
		CloseHandle(sync->consumerEvents[i]);
	}
	/*
	assert(sync->consumersEvent);
	CloseHandle(sync->consumersEvent);
	*/
	return TRUE;
}
#endif // VERSION == 'B'



#if VERSION == 'C'
BOOL initializeSyncStruct(LPSyncStruct sync) {
	InitializeCriticalSection(&sync->cs);
	sync->consumersFirstDoor = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (sync->consumersFirstDoor == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create consumersFirstDoor event. Error: %x\n"), GetLastError());
		return FALSE;
	}
	sync->consumersSecondDoor = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (sync->consumersSecondDoor == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create consumersSecondDoor event. Error: %x\n"), GetLastError());
		return FALSE;
	}
	sync->producerDoor = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (sync->producerDoor == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create producerDoor event. Error: %x\n"), GetLastError());
		return FALSE;
	}
	sync->threadInFirstRoom = 0;
	sync->threadInSecondRoom = 0;
	
	return TRUE;
}
BOOL producerWait(LPSyncStruct sync) {
	if (WaitForSingleObject(sync->producerDoor, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on producerDoor failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL producerSignal(LPSyncStruct sync) {
	if (!SetEvent(sync->consumersFirstDoor)) {
		_ftprintf(stderr, _T("Impossible to open the consumersFirstDoor. Error: %x"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL consumerWait(LPSyncStruct sync, INT threadNumber) {
	BOOL iAmTheLastToPassFirstDoor;
	if (WaitForSingleObject(sync->consumersFirstDoor, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumersFirstDoor failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	EnterCriticalSection(&sync->cs);
	iAmTheLastToPassFirstDoor = (++sync->threadInFirstRoom == 4);
	LeaveCriticalSection(&sync->cs);
	if (iAmTheLastToPassFirstDoor) {
		// i am the last, i must close the door and can open the second door (positioned after the job to be done)
		sync->threadInFirstRoom = 0;
		ResetEvent(sync->consumersFirstDoor);
		// now the thread that have done their job can go through the second door
		SetEvent(sync->consumersSecondDoor);
	}
	return TRUE;
}
BOOL consumerSignal(LPSyncStruct sync, INT threadNumber) {
	BOOL iAmTheLastToPassSecondDoor;
	if (WaitForSingleObject(sync->consumersSecondDoor, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumersSecondDoor failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	EnterCriticalSection(&sync->cs);
	iAmTheLastToPassSecondDoor = (++sync->threadInSecondRoom == 4);
	LeaveCriticalSection(&sync->cs);
	if (iAmTheLastToPassSecondDoor) {
		// i am the last, i must close the second door and can set the producer event
		sync->threadInSecondRoom = 0;
		ResetEvent(sync->consumersSecondDoor);
		// now the thread that have done their job can go through the second door
		SetEvent(sync->producerDoor);
	}
	return TRUE;
}
BOOL DestroySyncStruct(LPSyncStruct sync) {
	assert(sync->consumersFirstDoor);
	CloseHandle(sync->consumersFirstDoor);
	assert(sync->consumersSecondDoor);
	CloseHandle(sync->consumersSecondDoor);
	assert(sync->producerDoor);
	CloseHandle(sync->producerDoor);
	DeleteCriticalSection(&sync->cs);
	/*
	assert(sync->consumersEvent);
	CloseHandle(sync->consumersEvent);
	*/
	return TRUE;
}
#endif // VERSION == 'C'



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
