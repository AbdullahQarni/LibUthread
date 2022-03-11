#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <queue.h>

#define TEST_ASSERT(assert)				\
do {									\
	printf("ASSERT: " #assert " ... ");	\
	if (assert) {						\
		printf("PASS\n");				\
	} else	{							\
		printf("FAIL\n");				\
		exit(1);						\
	}									\
} while(0)

/* Create */
void test_create(void)
{
	fprintf(stderr, "*** TEST create ***\n");

	TEST_ASSERT(queue_create() != NULL);
}

/* Enqueue/Dequeue simple */
void test_queue_simple(void)
{
	int data = 3, *ptr;
	queue_t q;

	fprintf(stderr, "*** TEST queue_simple ***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_dequeue(q, (void**)&ptr);
	TEST_ASSERT(ptr == &data);
}

// Tests enqueueing and dequeueing multiple times.
// Make sure can still enqueue after the queue is emptied.
void test_queue_mult(void)
{
	int data1 = 1, *ptr1;
	int data2 = 2, *ptr2;
	int data3 = 3, *ptr3;
	int data4 = 4, *ptr4;
	int data5 = 5, *ptr5;
	int data6 = 6, *ptr6;
	int data7 = 7, *ptr7;

	queue_t q;

	fprintf(stderr, "*** TEST queue (first set of en/dequeues) ***\n");

	q = queue_create();

	queue_enqueue(q, &data1);
	queue_enqueue(q, &data2);
	queue_enqueue(q, &data3);
	queue_enqueue(q, &data4);

	queue_dequeue(q, (void**)&ptr1);
	queue_dequeue(q, (void**)&ptr2);
	queue_dequeue(q, (void**)&ptr3);
	queue_dequeue(q, (void**)&ptr4);

	TEST_ASSERT(ptr1 == &data1);
	TEST_ASSERT(ptr2 == &data2);
	TEST_ASSERT(ptr3 == &data3);
	TEST_ASSERT(ptr4 == &data4);

	//Testing second set of enqueues/dequeue after emptying 
	fprintf(stderr, "*** TEST queue (second set of en/dequeues) ***\n");

	queue_enqueue(q, &data5);
	queue_enqueue(q, &data6);
	queue_enqueue(q, &data7);

	queue_dequeue(q, (void**)&ptr5);
	queue_dequeue(q, (void**)&ptr6);
	queue_dequeue(q, (void**)&ptr7);

	TEST_ASSERT(ptr5 == &data5);
	TEST_ASSERT(ptr6 == &data6);
	TEST_ASSERT(ptr7 == &data7);
}

// Make sure can properly destroy an empty queue.
void test_destroy(void)
{	
	queue_t q;
	q = queue_create();

	fprintf(stderr, "*** TEST destroy ***\n");

	TEST_ASSERT(queue_destroy(q) == 0);
}

// Cannot destroy a queue with elements in it.
void test_destroy_fail(void)
{	
	int data = 3;
	queue_t q;
	q = queue_create();
	queue_enqueue(q, &data);
	
	fprintf(stderr, "*** TEST destroy fail when queue is not empty ***\n");

	TEST_ASSERT(queue_destroy(q) == -1);
}

// Test that the length of the queue follows adds and deletes.
void test_length(void)
{
	queue_t q;
	q = queue_create();

	fprintf(stderr, "*** TEST length when declared ***\n");
	TEST_ASSERT(queue_length(q) == 0);

	fprintf(stderr, "*** TEST length when not empty ***\n");
	int data1 = 1, *ptr1;
	int data2 = 2, *ptr2;
	int data3 = 3, *ptr3;

	queue_enqueue(q, &data1);
	queue_enqueue(q, &data2);
	queue_enqueue(q, &data3);

	TEST_ASSERT(queue_length(q) == 3);

	fprintf(stderr, "*** Test length when emptied ***\n");
	queue_dequeue(q, (void**)&ptr1);
	queue_dequeue(q, (void**)&ptr2);
	queue_dequeue(q, (void**)&ptr3);

	TEST_ASSERT(queue_length(q) == 0);
}

// Search for and delete items in the queue.
void test_delete(void)
{	
	int data1 = 3;
	int data2 = 4;
	queue_t q;
	q = queue_create();
	queue_enqueue(q, &data1);

	fprintf(stderr, "*** TEST delete when data is not in queue  ***\n");
	TEST_ASSERT(queue_delete(q, &data2) == -1);
	fprintf(stderr, "*** TEST delete when data is in queue  ***\n");
	TEST_ASSERT(queue_delete(q, &data1) == 0);
}

// Finds an entry with the same value are check.
int call_back(queue_t queue, void *data, void *check)
{
	(void) queue;
	if (*((int *)data) == *((int *)check)) 
	{
		return 1;
	}
	return 0;
}

// call_back, but deletes the queue entry upon a match instead.
int call_back_delete(queue_t queue, void *data, void *check)
{
	(void) queue;
	if (*((int *)data) == *((int *)check)) 
	{
		queue_delete (queue, data);
	}
	return 0;
}

// Uses call_back to find a value and copy it.
void test_queue_iterate_simple(void)
{
	int data1 = 3;
	int *ptr;
	int check = 3;
	queue_t q;
	q = queue_create();
	queue_enqueue(q, &data1);

	fprintf(stderr, "*** TEST simple iterate, makes sure that ptr stored found data  ***\n");
	queue_iterate(q, call_back, (void*)&check, (void**)&ptr);
	TEST_ASSERT(ptr == &data1);
}

// Check resistance to deletion.
// iterate should return 0 by reaching end of the queue.
void test_queue_iterate(void)
{
	int data1 = 1;
	int data2 = 2;
	int data3 = 3;
	int data4 = 4;
	int data5 = 5;
	int data6 = 6;
	int data7 = 7;

	int *ptr;
	int check = 4;

	queue_t q;

	q = queue_create();

	queue_enqueue(q, &data1);
	queue_enqueue(q, &data2);
	queue_enqueue(q, &data3);
	queue_enqueue(q, &data4);
	queue_enqueue(q, &data5);
	queue_enqueue(q, &data6);
	queue_enqueue(q, &data7);

	fprintf(stderr, "*** TEST iterate, when check is in middle ***\n");
	queue_iterate(q, call_back, (void*)&check, (void**)&ptr);
	TEST_ASSERT(ptr == &data4);
	fprintf(stderr, "*** TEST iterate, when check is deleted ***\n");
	TEST_ASSERT(queue_iterate(q, call_back_delete, (void*)&check, (void**)&ptr) == 0);
	TEST_ASSERT(queue_length(q) == 6);
}


int main(void)
{
	test_create();
	test_queue_simple();
	test_queue_mult();
	test_destroy();
	test_destroy_fail();
	test_length();
	test_delete();
	test_queue_iterate_simple();
	test_queue_iterate();

	return 0;
}
