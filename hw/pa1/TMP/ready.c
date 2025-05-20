/* ready.c - ready */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lab1.h"

/*------------------------------------------------------------------------
 * ready  --  make a process eligible for CPU service
 *------------------------------------------------------------------------
 */
int ready(int pid, int resch)
{
	register struct	pentry	*pptr;

	if (isbadpid(pid))
		return(SYSERR);
	pptr = &proctab[pid];
	pptr->pstate = PRREADY;

	int current_scheduling = getschedclass();
	if (current_scheduling == AGESCHED) {
		// Do aging scheduling
		insert(pid,rdyhead,pptr->aged_priority);
	} else if (current_scheduling == LINUXSCHED) {
		// Do linux like scheduling
		insert(pid,rdyhead,pptr->goodness);
	} else {
		// Default XINU scheduling
		insert(pid,rdyhead,pptr->pprio);
	} 

	if (resch)
		resched();
	return(OK);
}
