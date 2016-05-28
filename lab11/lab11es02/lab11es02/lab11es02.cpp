/*
Lab 11 exercise 02

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

INT _tmain(INT argc, LPTSTR argv[]) {

/*
	init(full, 0);
	init(empty, N);
	init(MEp, 1);
	init(MEc, 1);
*/

	return 0;
}

/*
Producer() {
	Message m;
	while (TRUE) {
		produce m;
		wait(empty);
		wait(MEp);
		enqueue(m);
		signal(MEp);
		signal(full);
	}
}

Consumer() {
	Message m;
	while (TRUE) {
		wait(full);
		wait(MEc);
		m = dequeue();
		signal(MEc);
		signal(empty)
			print - out m;
	}
}
*/