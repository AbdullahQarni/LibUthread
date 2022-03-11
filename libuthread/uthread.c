#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "queue.h"
#include "uthread.h"

// State variable for thread status.
enum thread_state
{
	READY,
	RUNNING,
	BLOCKED,
	ZOMBIE
};

// Thread information storage.
struct TCB
{
	// Execution context.
	uthread_t TID;
	int status;
	void *stack;
	uthread_ctx_t *context;
	// Joining information.
	struct TCB* joiner;
	int return_value;
	// If the thread needs to collect from a zombie.
	int collector;
};

// Scheduler queue and data structures to hold zombies.
queue_t scheduler;
queue_t zombie_q;
// The currently running thread.
struct TCB *cur_thread = NULL;
// The main thread.
struct TCB *main_thread = NULL;
// Keep track of TID numbers.
uthread_t num_thread;

int uthread_start(int preempt)
{
	// Set up a TCB for the main thread.
	main_thread = malloc(sizeof(struct TCB));
	if (main_thread == NULL) return -1;

	// Prepare the scheduler and zombie queues.
	scheduler = queue_create();
	if (scheduler == NULL) return -1;
	zombie_q = queue_create();
	if (zombie_q == NULL) return -1;

	// Initialize thread identity information.
	main_thread->TID = 0;
	main_thread->context = malloc(sizeof(uthread_ctx_t));

	// Initialize joining information.
	main_thread->joiner = NULL;
	main_thread->return_value = 0;
	main_thread->collector = 0;

	// The main thread is also the only thread running at the moment.
	main_thread->status = RUNNING;
	cur_thread = main_thread;
	num_thread = 0;

	// Toggle preemption.
	if (preempt)
	{
		preempt_start();
		// Since main never calls ctx_init, must enable ourselves.
		preempt_enable();
	}
 	return 0;
}

int uthread_stop(void)
{
	// Main must be running.
	if (cur_thread != main_thread) 
		return -1;

	// If there are user threads remaining then user error due to them not joining them all.
	// Account for 1 in case of main in scheduler, since main doesn't call uthread_exit.
	if (queue_length(scheduler) > 1 || queue_length(zombie_q)) return -1;

	preempt_stop();

	// Remove main thread if it's still there to allow scheduler to be freed.
	queue_delete(scheduler, (void*) main_thread);

	// Stop the scheduler.
	queue_destroy(scheduler);
	queue_destroy(zombie_q);
	scheduler = NULL;
	zombie_q = NULL;

	// Main thread no longer needed.
	free(main_thread->context);
	free(main_thread);
	cur_thread = NULL;
	main_thread = NULL;
	return 0;
}

int uthread_create(uthread_func_t func)
{
	// Do not force yield in the middle of initializing a new thread.
	preempt_disable();
	// TID overflow check.
	if (num_thread == USHRT_MAX)
		return -1;
	num_thread++;

	// Allocate a TCB for the new thread.
	struct TCB *new_thread = malloc(sizeof(struct TCB));
	if (new_thread == NULL)
		return -1;

	// Initialize execution context of the new thread.
	new_thread->TID = num_thread;
	new_thread->status = READY;
	new_thread->stack = uthread_ctx_alloc_stack();
	new_thread->context = malloc(sizeof(uthread_ctx_t));
	if (uthread_ctx_init(new_thread->context, new_thread->stack, func))
		return -1;

	// Initialize joining information.
	new_thread->joiner = NULL;
	new_thread->return_value = 0;
	new_thread->collector = 0;

	queue_enqueue(scheduler, new_thread);
	preempt_enable();
	return new_thread->TID;
}

void uthread_yield(void)
{
	// Do not force yield while the process is yielding already.
	preempt_disable();
	// Prevent threads from yielding onto themselves.
	if (queue_length(scheduler) || cur_thread->status == ZOMBIE)
	{
		// Save current thread info for context switching.
		struct TCB *prev_thread = cur_thread;

		// Pause the current thread and put it back into the scheduler.
		if (cur_thread->status == RUNNING)
		{
			cur_thread->status = READY;
			queue_enqueue(scheduler, cur_thread);
		}

		// Get the oldest ready thread, and set that to be the current running thread.
		// If no ready user threads, default to main.
		if (queue_length(scheduler))
			queue_dequeue(scheduler, (void**) &cur_thread);
		else
			cur_thread = main_thread;
		cur_thread->status = RUNNING;

		// Switch context, then allow preemption on the way back.
		// Except for threads that return to collecting in join since they edit data.
		uthread_ctx_switch(prev_thread->context, cur_thread->context);
		if (!cur_thread->collector) preempt_enable();
	}
}

uthread_t uthread_self(void)
{
	// If there's no thread running can't return anything.
	if (cur_thread == NULL)
		return -1;
	return cur_thread->TID;
}

void uthread_exit(int retval)
{
	// Do not force yield while cur_thread and the queue are being edited.
	preempt_disable();
	cur_thread->status = ZOMBIE;
	cur_thread->return_value = retval;
	// If thread is joined, unblock its joiner.
	if (cur_thread->joiner != NULL)
	{
		// Move joiner to the end of the ready queue.
		cur_thread->joiner->status = READY;
		queue_enqueue(scheduler, cur_thread->joiner);
	} else {
		// Add to zombie queue to be freed later when joined.
		// If already joined, joiner will free when they resume.
		queue_enqueue(zombie_q, cur_thread);
	}
	uthread_yield();
}

// Searches for a thread within a particular queue by TID
int uthread_search(queue_t queue, void *incoming, void *check)
{
	(void) queue;
	if (((struct TCB*) incoming)->TID == *(uthread_t*) check) return 1;
	return 0;
}

int uthread_join(uthread_t tid, int *retval)
{
	// Do not change the makeup of the queue while searching for tid.
	preempt_disable();
	struct TCB *child = NULL;
	// Search for the tid within the scheduler. If not there, check to see if it's a zombie.
	queue_iterate(scheduler, uthread_search, (void*) &tid, (void**) &child);
	if (child == NULL) queue_iterate(zombie_q, uthread_search, (void*) &tid, (void**) &child);
	// tid doesn't exist.
	if (child == NULL) return -1;
	// tid is main, the calling thread, or already joined.
	if (tid == 0 || tid == cur_thread->TID || child->joiner != NULL) 
		return -1;

	// If the child is a zombie, collect its return status and move on.
	if (child->status == ZOMBIE)
	{
		if (retval != NULL) *retval = child->return_value;
		queue_delete(zombie_q, child);
	}
	else {
		// Block the current thread and yield.
		child->joiner = cur_thread;
		cur_thread->status = BLOCKED;
		cur_thread->collector = 1;
		uthread_yield();
		// We're back, collect then terminate the joined thread.
		cur_thread->collector = 0;
		if (retval != NULL) *retval = child->return_value;
	}
	uthread_ctx_destroy_stack(child->stack);
	free(child->context);
	free(child);
	preempt_enable();
	return 0;
}
