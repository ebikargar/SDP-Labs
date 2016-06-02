/*
Lab 12 exercise 01

This program is based on the model of producers and consumers. In this case there is a single producer
that reads a file where LONG are stored. All of the consumers receive the same data from the producer
but each of them processes it in a different way.
The synchronization primitives used are semaphores (version A) and events (version B and C).
While the implementation with semaphores is straightforward, the events cause some problems: the pulse
used on MANUAL_RESET events does not guarantee the right synchronization mechanism, because we can't
be sure that all the consumers are already waiting on the event.
Also the function SignalSemaphoreAndWait does not guarantee atomicity between the signal of the semaphore
and the wait on the event; it only reduces probability to have those two actions separated, by being faster
in the execution than doing the two calls separately.
For this reason the version C uses a two-barriers system, implemented with MANUAL_RESET events which are
set and never pulsed. Two barriers are needed since the threads are cyclic.

@Author: Martino Mensio
*/

// change there for choosing the version
#define VERSION 'C'


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
		yet waiting on the event, they won't be able to go on after going to wait, also because the producer will
		be waiting for it on his own event/semaphore.
		Doing it with a MANUAL_RESET together with a SetEvent is risky because some fast consumer can iterate more
		times while the event is signalled. To fix that, it is possible to use a 2-barriers mechanism in which the
		last thread of each barrier resets the event (see version C).
	*/
} SyncStructB;


/*
	consumers flow in version C:
	+-----------------------+---------------------------------+-----------------------+
	                        |                                 |
	    prelude room     door 1     room 1 (work there)     door 2     out (room 2)
	                        |                                 |
	+-----------------------+---------------------------------+-----------------------+

	Door 1 is opened by the producer, to allow consumer to enter room 1 and do their job with the data.
	Door 1 is closed by the last consumer passing through it.
	Door 2 is opened by the last consumer entering room 1 (the one that has already closed door 1).
	Door 2 is closed by the last consumer going out.
	Producer door is opened by the last consumer going out.
	Producer door is closed automatically (AUTO_RESET event).
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
	INT threadNumber;	// who am i?
	LPLONG n;			// pointer to shared variable (value read)
	LPBOOL done;		// pointer to shared flag (termination)
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
	// open file for reading
	hIn = CreateFile(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to open input file. Error: %x\n"), GetLastError());
		return 2;
	}
	// initialize the structure containing semaphores/events
	if (!initializeSyncStruct(&sync)) {
		return 3;
	}
	// create the threads
	for (i = 0; i < 4; i++) {
		// copy a pointer to the sync structure (only one, shared)
		param[i].sync = &sync;
		param[i].threadNumber = i;
		// copy a pointer to the variable and to the flag
		param[i].n = &number;
		param[i].done = &done;
		consumerThreads[i] = CreateThread(NULL, 0, consumer, &param[i], 0, NULL);
		if (consumerThreads[i] == INVALID_HANDLE_VALUE) {
			_ftprintf(stderr, _T("Impossible to create consumer thred. Error: %x\n"), GetLastError());
			return 4;
		}
	}
	// the file is not yet finished
	done = FALSE;

	// start feeding the threads
	while (ReadFile(hIn, &number, sizeof(number), &nRead, NULL) && nRead > 0) {
		// check the count of read bytes
		if (nRead != sizeof(number)) {
			_ftprintf(stderr, _T("Record size mismatch\n"));
			break;
		}
		_tprintf(_T("Producer value: %ld\n"), number);
		// wake up the consumers
		if (!producerSignal(&sync)) {
			return 5;
		}
		// wait for the consumers
		if (!producerWait(&sync)) {
			return 6;
		}
	}
	// file finished
	done = TRUE;
	// wake up the consumers, that will see that read is terminated
	if (!producerSignal(&sync)) {
		return 7;
	}

	CloseHandle(hIn);
	
	if (WaitForMultipleObjects(4, consumerThreads, TRUE, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait threads failed. Error: %x\n"), GetLastError());
		return 8;
	}

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
		// thread 1 needs to calculate the partial product, so starts from 1 (neutral value for multiplication)
		accumulator = 1;
	}
	while(TRUE) {
		// wait for the producer
		if (!consumerWait(param->sync, param->threadNumber)) {
			return 1;
		}
		// check if producer has done with the read
		if (*param->done) {
			break;
		}
		// do the job
		switch (param->threadNumber) {
		case 0:
			accumulator += *param->n;
			_tprintf(_T("Thread %d (partial sum) value: %lld\n"), param->threadNumber, accumulator);
			break;
		case 1:
			accumulator *= *param->n;
			// TODO manage overflow
			_tprintf(_T("Thread %d (partial product) value: %lld\n"), param->threadNumber, accumulator);
			break;
		case 2:
			resultNumber = factorial(*param->n);
			_tprintf(_T("Thread %d (factorial) value: %s\n"), param->threadNumber, resultNumber);
			free(resultNumber);
			break;
		case 3:
			stringResult = (LPTSTR)calloc(*param->n + 1, sizeof(TCHAR));
			for (i = 0; i < *param->n; i++) {
				stringResult[i] = _T('#');
			}
			stringResult[i] = 0;
			_tprintf(_T("%s\n"), stringResult);
			free(stringResult);
			break;
		default:
			// should not get here
			_ftprintf(stderr, _T("are you crazy?\n"));
			break;
		}
		// wake up the producer
		if (!consumerSignal(param->sync, param->threadNumber)) {
			return 2;
		}
	}
	
	return 0;
}



#if VERSION == 'A'
BOOL initializeSyncStruct(LPSyncStruct sync) {
	INT i;
	// create a semaphore for the producer to wait on: values from 0 to 4
	sync->producerSem = CreateSemaphore(NULL, 0, 4, NULL);
	if (sync->producerSem == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create producer semaphore. Error: %x\n"), GetLastError());
		return FALSE;
	}
	for (i = 0; i < 4; i++) {
		// create a semaphore for each consumer: values from 0 to 1
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
		// wait 4 times on the semaphore (one for each consumer)
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
		// wake up each consumer (on its own semaphore)
		if (!ReleaseSemaphore(sync->consumerSems[i], 1, NULL)) {
			_ftprintf(stderr, _T("Error releasing consumer semaphore. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL consumerWait(LPSyncStruct sync, INT threadNumber) {
	// wait on its own semaphore
	if (WaitForSingleObject(sync->consumerSems[threadNumber], INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumer semaphore failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL consumerSignal(LPSyncStruct sync, INT threadNumber) {
	// signal on the producer semaphore
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
	// for each consumer create two events, they are used as binary semaphores. Each event is AUTO_RESET
	for (i = 0; i < 4; i++) {
		sync->consumerEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
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
	// wait for the outgoing events of all of the consumers
	if (WaitForMultipleObjects(4, sync->producerEvents, TRUE, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on producer events failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL producerSignal(LPSyncStruct sync) {
	INT i;
	for (i = 0; i < 4; i++) {
		// setEvent on incoming event of each of the consumers
		if (!SetEvent(sync->consumerEvents[i])) {
			_ftprintf(stderr, _T("Error setting consumer event. Error: %x\n"), GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}
BOOL consumerWait(LPSyncStruct sync, INT threadNumber) {
	// wait on the own incoming event
	if (WaitForSingleObject(sync->consumerEvents[threadNumber], INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumer event failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL consumerSignal(LPSyncStruct sync, INT threadNumber) {
	// setEvent on the own outgoing event
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
	return TRUE;
}
#endif // VERSION == 'B'



#if VERSION == 'C'
BOOL initializeSyncStruct(LPSyncStruct sync) {
	InitializeCriticalSection(&sync->cs);
	// create the three events: consumersDoors are MANUAL_RESET
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
	// producerDoor is AUTO_RESET
	sync->producerDoor = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (sync->producerDoor == INVALID_HANDLE_VALUE) {
		_ftprintf(stderr, _T("Impossible to create producerDoor event. Error: %x\n"), GetLastError());
		return FALSE;
	}
	// initialize counters
	sync->threadInFirstRoom = 0;
	sync->threadInSecondRoom = 0;
	
	return TRUE;
}
BOOL producerWait(LPSyncStruct sync) {
	// wait on the producerDoor event, that will be set by the last thread that has finished working on the data
	if (WaitForSingleObject(sync->producerDoor, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on producerDoor failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	// since this is AUTO_RESET event, after the wait the producerDoor is closed
	return TRUE;
}
BOOL producerSignal(LPSyncStruct sync) {
	// let the consumers enter the room where the data to be processed is
	if (!SetEvent(sync->consumersFirstDoor)) {
		_ftprintf(stderr, _T("Impossible to open the consumersFirstDoor. Error: %x"), GetLastError());
		return FALSE;
	}
	return TRUE;
}
BOOL consumerWait(LPSyncStruct sync, INT threadNumber) {
	BOOL iAmTheLastToPassFirstDoor;
	// wait that the first door is opened
	if (WaitForSingleObject(sync->consumersFirstDoor, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumersFirstDoor failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	// now the consumer is in the room where the data is
	EnterCriticalSection(&sync->cs);
	// update the counter in this room and check if i was the last one to arrive
	iAmTheLastToPassFirstDoor = (++sync->threadInFirstRoom == 4);
	LeaveCriticalSection(&sync->cs);
	if (iAmTheLastToPassFirstDoor) {
		// no one else is arriving, so no one needs to know if it the last one
		sync->threadInFirstRoom = 0;
		// i am the last, i must close the firs door before opening the other one, so that no quick consumer can enter again
		ResetEvent(sync->consumersFirstDoor);
		// and now i can open the second door (positioned after the job to be done) so that thread that have done their job can go out
		SetEvent(sync->consumersSecondDoor);
	}
	return TRUE;
}
BOOL consumerSignal(LPSyncStruct sync, INT threadNumber) {
	BOOL iAmTheLastToPassSecondDoor;
	// need the second door open to go out
	if (WaitForSingleObject(sync->consumersSecondDoor, INFINITE) == WAIT_FAILED) {
		_ftprintf(stderr, _T("Wait on consumersSecondDoor failed. Error: %x\n"), GetLastError());
		return FALSE;
	}
	// now consumer is out of room 1
	EnterCriticalSection(&sync->cs);
	// update the counter in this room and check if i was the last one to arrive
	iAmTheLastToPassSecondDoor = (++sync->threadInSecondRoom == 4);
	LeaveCriticalSection(&sync->cs);
	if (iAmTheLastToPassSecondDoor) {
		// i am the last
		sync->threadInSecondRoom = 0;
		// i must close the second door
		ResetEvent(sync->consumersSecondDoor);
		// and can set the producer event because i am sure that no one is inside the room with data
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
	return TRUE;
}
#endif // VERSION == 'C'


/*
	below functions are small modifications from http://www.geeksforgeeks.org/factorial-large-number/
	to handle numbers which don't fit in a LONGLONG. For this reason, an array of digits is used, to represent
	numbers in a way similar to BCD (very inefficient and mind-blowing, but fast to implement)
*/

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
