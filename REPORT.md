# Project 2: User Thread Library
## Overview
The goal of this project was to create a fully functional user thread library. The control flow of the project is as follows.
1. Implement a queue API that stores data in FIFO order.
2. Implement the threading API, applying the queue for scheduling.
3. Add preemption using knowledge about signals and timers.

## Queue
We decided to use a linked-list approach over a dynamic array to represent the
queue. A struct `q_entry` was created to act as a "node" within the linked
list. It contains a pointer to a data object and the `next` and `prev` nodes
in the list. The queue itself is `struct queue`, and contains the length, and
an `oldest` and `newest` `q_entry` to represent the first and last items. 

Constantly having to reallocate the size of an array to account for
enqueues and deletions was deemed inefficient and perhaps not optimal to the
requirements of O(1) time complexity for some commands. Furthermore, in those
cases of deletions, there was no logical way to "bridge the gap" between entries
besides the inefficient move of shifting every entry to the left.

The advantage of the linked list implementation is its modularity and
efficiency. For example, when deleting items in the queue, "bridging the gap"
was simply changing the links between adjacent nodes and did not require
shifting entire parts of the list. Enqueueing and dequeueing items involved
modifying the `oldest` and `newest` elements of the queue, then simply changing
the links of the item directly before or after the item. Rather than reallocate
the size of the entire list, it made more sense to allocate and free space for
each individual `q_entry` instead.

One particular challenge with this implementation was making `queue_iterate`
resistant to deletion. One prior solution was, after the calling function
returns, we would iterate back over the list to find the first non-NULL node
that has not been called on. However, this solution is extremely inefficient
due to iterating over the list for every single function call. We instead came
to a compromise solution. Before running the callback function, we would save
the next node into a local variable. If the current node gets deleted, the
loop would not get lost since it has the next node. This is limited as it only
makes `queue_iterate` resistant to the deletion of the current `q_entry`.

`queue_tester.c` contains our unit tests for this phase, and tests the
functionality and input error checking of each queue function.

## Threading
Threads are represented as a `struct TCB` that contains its context and joining
information. We had two global `TCB`s outside of our queues. `cur_thread` is
the currently running thread, and `main_thread` is the calling process of 
`uthread_start`. This allowed us to keep track of them as there are cases where
either of them are not in a queue.

A single queue system for the scheduler was inefficient. In `uthread_yield`, if
we wanted to find the first thread with `status == READY`, we would have to
call `queue_iterate` every time, since the queue could contain threads of every
status. In the end, we used two queues: `scheduler` for ready threads, and
`zombie_q` for zombies. `scheduler` allows for the simple use of `dequeue` to
get the next thread to yield to. Threads can exit before being joined. As such,
we had to have a place, `zombie_q`, to store them without cluttering the
scheduler. This queue would later be accessed by `uthread_join` when searching
for zombies by TID.

Earlier builds had a `blocked_q` for blocked threads. However, enqueueing them
in `uthread_join`, then `queue_delete`ing them from the `blocked_q` and putting
them back in `scheduler` was not a good use of time. Instead, we added a
`struct TCB *` called `joiner` to the TCB struct. This `joiner` is set in
`uthread_join` by the `cur_thread` into the joined thread. The joined thread is
found by using `queue_iterate` to find a TCB by TID. When a joined thread
exits, we can simply enqueue `joiner` back into `scheduler.` This also allowed
us to check if a particular thread is joined by checking if `joiner` is `NULL`.

We believe that `uthread_hello` and `uthread_yield` were sufficient enough to
show that the general functionality of threading works. Modifications were made
to `uthread_yield`, called `uthread_return`, to comply with the requirements of
phase 3. Instead of joining with `NULL`, we had `main` join to `thread1`,
`thread2`, and `thread3` with non-NULL `retval` pointers (in `uthread_yield`,
`thread2` and `thread3` are never joined by the user and stay as zombies,
causing memory leaks and failed `queue_destroy`). This test shows that
`uthread_join` and `uthread_exit` work as intended by printing the
return values of the three threads in `main`.

## Preemption
Preemption was done using sigactions and timers. We set up a sigaction that
responded to `SIGVTALRM` with a handler called `preempt`. `preempt` simply
calls a forced `uthread_yield`. We used a `sigset` that stored
the `SIGVTALRM` signal. With `sigprocmask` we could block and unblock
`SIGVTALRM` using the `sigset` as an argument. Finally, in order to raise
`SIGVTALRM`, we used a virtual timer that uses an `itimerval` with an interval
of 10000 microseconds. In `uthread.c` we made sure to disable preemption in
cases that would edit global variables like queues, the current running thread,
and the main thread.

The fact that `sigaction`, `sigprocmask`, and `setitimer` all have an option to
save the previous configuration into a pointer was taken advantage of for
`preempt_stop`. By saving the configurations into global pointers in
`preempt_start`, all we have to do to restore the previous states in
`preempt_stop` is call `sigprocmask`, `sigaction`, and `setitimer` with these
pointers that store the old configurations.

Preemption was tested in two ways. First, `uthread_return` was run with
preemption turned on multiple times in valgrind. Ignoring valgrind errors, we 
made sure that the output of the program itself was the same every time and
that there were no memory leaks due to preempting at improper times. Second, 
the tester `test_preempt.c` runs a thread that has an infinite loop (main is 
blocked on this thread). With preemption, the thread eventually yields to 
a second thread that breaks the loop. Then, the first thread dies and is
collected by main. The second thread prints its message before the first one,
showing that the second was the first to return. Without preemption, 
`test_preempt` hangs.

## Limitations
Due to the implementation of saving the next element in the queue before using
the callback function, `queue_iterate` is not resistant to the deletion of the
item directly `next` to the current `q_entry` being iterated on.

The thread library assumes that user input is correct, and while there are
error checks that return -1, there may be unexpected behavior and memory leaks
due to user error.

Having no queue to store blocked threads means that threads cannot join them,
since we cannot use `queue_iterate` to find them.

The preemption system is inconsistent. We are not sure if `sigprocmask` is
actually working. A previous build literally removed the handler from the
sigaction, but we felt this fell out of the scope of the problem. In one test,
we could not escape an infinite loop using a secondary thread to change a
global variable to break the loop unless a `printf` statement was involved.
Places where we disabled preemption in `uthread.c` are suboptimal at best.

We were confused by the given `HZ` value since the GNU documentation for 
`timeval` says the units are in seconds and microseconds. In the end, we
converted to microseconds, using a value of 10000 (10000 microseconds = .01
seconds) and used that value for the timer interval.

There are a wide array of unsolved errors that appear on valgrind when
executing the test programs like invalid reads and writes. However, as long as
the user's input is correct, we have made sure that at the very least the
output is consistent and that there are no memory leaks (unless in extremely
rare occasions that we did not catch).

### References
Makefile.pdf, Makefile_v3.0, signal.c, contexts_fixed_schedule.c

Example code provided in project specs.

Piazza  @147, @179, @209, @233, @274, @293.

GNU and IBM documentation on `sigaction`, `sighandler`, `sigset`, 
`sigprocmask`, `itimerval`, and `timeval`. Various man pages.
