/* scount.c - scount */

#include <conf.h>
#include <kernel.h>
#include <sem.h>
#include <proc.h>
#include "lab0.h"
/*------------------------------------------------------------------------
 *  scount  --  return a semaphore count
 *------------------------------------------------------------------------
 */
SYSCALL scount(int sem)
{
	syscall_started(currpid, SYSCALL_SCOUNT);
extern	struct	sentry	semaph[];
	if (isbadsem(sem) || semaph[sem].sstate==SFREE)
		return(SYSERR);
	syscall_ended(currpid, SYSCALL_SCOUNT);
	return(semaph[sem].semcnt);
}
