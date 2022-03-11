#include <stdio.h>
#include <stdlib.h>
#include <uthread.h>

/*
uthread_return. This is uthread_yield except all of the threads are joined
with non-null retvals. This should print their return values in main without
depending on the threads to do it themselves.
*/

uthread_t two_id, three_id;

int thread3(void)
{
	uthread_yield();
	printf("thread%d\n", uthread_self());
	return 7;
}

int thread2(void)
{
	three_id = uthread_create(thread3);
	uthread_yield();
	printf("thread%d\n", uthread_self());
	return 6;
}

int thread1(void)
{
	two_id = uthread_create(thread2);
	uthread_yield();
	printf("thread%d\n", uthread_self());
	uthread_yield();
	return 5;
}

int main(void)
{
	int one, two, three;
	uthread_start(0);
	uthread_join(uthread_create(thread1), &one);
	uthread_join(two_id, &two);
	uthread_join(three_id, &three);
	uthread_stop();

	printf("thread1 finished with return value %d.\n", one);
	printf("thread2 finished with return value %d.\n", two);
	printf("thread3 finished with return value %d.\n", three);
	return 0;
}
