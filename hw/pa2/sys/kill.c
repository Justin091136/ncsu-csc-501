/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}

    int lockid;
    int need_resched = 0;
    for (lockid = 0; lockid < NLOCKS; lockid++) {
        if (pptr->lock_list[lockid] == ACQUIRED) {
			release_lock(lockid, pid);
            need_resched = 1;
        }
    }

	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {
		case PRCURR:	
			pptr->pstate = PRFREE;	/* suicide */
			need_resched = 0;
			resched();

		case PRWAIT:
			semaph[pptr->psem].semcnt++;
			lockid = pptr->waiting_lock_id;
			if (is_lock_id_valid(lockid)) {
				pptr->pinh = 0;
				pptr->waiting_lock_id = -1;
				pptr->lock_return_val = DELETED;
				pptr->wait_time_start = -1;
				pptr->wait_type = -1;
				dequeue(pid);
				// Lock info may need adjustment
				locktab[lockid].lprio = get_max_prio_waiting_in_lock(lockid);
				propagate_priority_inheritance(lockid);
			}

		case PRREADY:	
			dequeue(pid);
			pptr->pstate = PRFREE;
			break;

		case PRSLEEP:
		case PRTRECV:
			unsleep(pid);
		default:	/* fall through	*/
			pptr->pstate = PRFREE;
	}

	if(need_resched) {
		resched();
	}

	restore(ps);
	return(OK);
}
