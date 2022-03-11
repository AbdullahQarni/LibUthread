#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uthread.h>

uthread_t one_tid;
uthread_t two_tid;
int x = 1;

/*
Preemption test. Main creates thread1 and thread2, then joins to thread1 and gets blocked.
thread1 runs in an infinite loop and stays that way with preemption off, hanging the test.
However, with preemption on, thread1 will force yield to thread2, which changes a value
such that thread1's loop breaks, allowing thread1 to die and be collected.
*/

// Second thread, to which when yielded to by preemption, should break thread1's loop.
int thread2(void)
{
	printf("Here!\n");
	x = 0;
	return 776;
}

// First thread, runs an infinite loop unless it is preempted, which yields to thread2.
// The loop prints a zero width space as to not fill up the terminal.
int thread1(void)
{
	while (x) printf("​");
	printf("There!\n");
	return 1000;
}

int main()
{
	// TID's to join and return values of those TIDs.
	int one_return = 0;
	int two_return = 0;

	// Threading process.
	uthread_start(1);
	one_tid = uthread_create(thread1);
	two_tid = uthread_create(thread2);
	uthread_join(one_tid, &one_return);
	uthread_join(two_tid, &two_return);
	uthread_stop();

	// Verify that both threads returned properly.
	printf("Everywhere!\n");
	printf("thread1 was collected with exit value %d\n", one_return);
	printf("thread2 was collected with exit value %d\n", two_return);
	return 0;
}
