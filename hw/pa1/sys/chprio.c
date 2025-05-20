/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include "lab1.h"

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
		(pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}

	// Update the priority
	pptr->pprio = newprio;

	int current_scheduling = getschedclass();
	if (current_scheduling == AGESCHED) {
		// Do aging scheduling
		pptr->aged_priority = newprio;  // Reset aged priority to the new priority

		if (pptr->pstate == PRREADY) {
			// Since the priority is modified, we need to reorganize the order of the queue
			dequeue(pid);
			insert(pid, rdyhead, pptr->aged_priority);
		}

	} else if (current_scheduling == LINUXSCHED) {
		// Do linux like scheduling
		// Do nothing, since the priority change will only take effect in the next epoch
	}

	restore(ps);
	return(newprio);
}
