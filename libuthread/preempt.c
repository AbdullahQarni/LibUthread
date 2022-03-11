#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"

// signal action that triggers a forced yield.
struct sigaction preempt_now;
struct sigaction preempt_never;
// sigset that controls blocking SIGVTALRM.
sigset_t preemption_blocker;
sigset_t old_blocker;
// timer that rings SIGVTALRM 100 times per second.
struct itimerval ringer;
struct itimerval old_ringer;

void preempt(int signum)
{
	(void) signum;
	uthread_yield();
}

void preempt_start(void) {
	// Prepare the blocker to toggle responses to SIGVTALRM.
	sigemptyset(&preemption_blocker);
	sigaddset(&preemption_blocker, SIGVTALRM);
	// Initially starts off blocked and is enabled in ctx_bootstrap or uthread_start.
	sigprocmask(SIG_SETMASK, &preemption_blocker, &old_blocker);

	// Establish a signal handler to yield when SIGVTALRM is raised.
	preempt_now.sa_handler = preempt;
	sigaction(SIGVTALRM, &preempt_now, &preempt_never);

	// Establish a timer that rings SIGVTARLM 100 times a second.
	// 100 times a second -> .01 seconds = 10000 microseconds.
	ringer.it_interval.tv_sec = 0;
	ringer.it_interval.tv_usec = 10000;
	ringer.it_value.tv_sec = 0;
	ringer.it_value.tv_usec = 10000;
	// Activate the timer based on the settings from start.
	// Save original settings to restore in stop.
	setitimer(ITIMER_VIRTUAL, &ringer, &old_ringer);
}

void preempt_stop(void)
{
	// Disable preemption so no forced yields while resetting.
	preempt_disable();
	// Restore response system to its original state.
	sigprocmask(SIG_SETMASK, &old_blocker, NULL);
	sigaction(SIGVTALRM, &preempt_never, NULL);
	setitimer(ITIMER_VIRTUAL, &old_ringer, NULL);
}

void preempt_enable(void)
{
	sigprocmask(SIG_UNBLOCK, &preemption_blocker, NULL);
}

void preempt_disable(void)
{
	sigprocmask(SIG_BLOCK, &preemption_blocker, NULL);
}
