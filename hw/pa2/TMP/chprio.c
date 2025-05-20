/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include "lock.h"

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

	pptr->pprio = newprio;
	update_inherited_priority(pid);

	if (pptr->pstate == PRREADY) {
        dequeue(pid);
        insert(pid, rdyhead, get_process_priority(pid));
    }

    int lockid = pptr->waiting_lock_id;
    if (is_lock_id_valid(lockid)) {
        locktab[lockid].lprio = get_max_prio_waiting_in_lock(lockid);
        propagate_priority_inheritance(lockid);
    }

	if (currpid == pid) {
        resched();
    }

	restore(ps);
	return(newprio);
}
