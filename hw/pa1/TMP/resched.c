/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lab1.h"


#define AGING_INCREMENT 2

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
int scheduler_class = DEFAULTSCHED;


void setschedclass(int scheduler_class_type)
{
	scheduler_class = scheduler_class_type;
}

int getschedclass()
{
	return scheduler_class;
}

/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */

void _print_current_priority_status()
{
	kprintf("current(%d) = %d, ", currpid, proctab[currpid].aged_priority);

	int cur = q[rdyhead].qnext;
	while (cur != rdytail) { 
		int pid = cur;

		if (pid == NULLPROC) {
			cur = q[cur].qnext;
			continue;
		}
		kprintf("[(%d):%d] ", pid, q[pid].qkey);

		cur = q[cur].qnext;
	}
	kprintf("\n");
}

void _print_goodness(char *starting_message)
{
	int has_print_header = 0;
	int is_table_empty = 1;
	int i;
   	for (i = 0; i < NPROC; i++) {
		if (i == NULLPROC || proctab[i].pstate == PRFREE) {
			continue;
		}
		if (!has_print_header) {
			kprintf("[New epoch] %s:", starting_message);
			has_print_header = 1;
		}
		is_table_empty = 0;
		kprintf(" (%d):%d", i, proctab[i].goodness );
	}
	if (!is_table_empty) {
		kprintf("\n\n");
	}
}

void next_epoch()
{
	// Remove every process from ready queue first, since their goodness may be changed soon
    int cur = q[rdyhead].qnext;
    while (cur != rdytail) {
        int pid = cur;
        cur = q[cur].qnext;
        if (pid != NULLPROC) {
            dequeue(pid);
        }
    }
    
    int i;
   	for (i = 0; i < NPROC; i++) {
		if (i == NULLPROC || proctab[i].pstate == PRFREE) {
			continue;
		}

		if (proctab[i].total_quantum == 0 || proctab[i].quantum_left == 0) {
			proctab[i].total_quantum = proctab[i].pprio;
		} else {
			proctab[i].total_quantum = (proctab[i].quantum_left / 2) + proctab[i].pprio;
		}

		proctab[i].quantum_left = proctab[i].total_quantum;
		proctab[i].goodness = proctab[i].quantum_left + proctab[i].pprio;

		if (proctab[i].pstate == PRREADY) {
			// Re-insert the ready process back into ready queue
			insert(i, rdyhead, proctab[i].goodness);
		}
	}
}


int resched()
{
	int current_scheduling = getschedclass();
	if (current_scheduling == AGESCHED) {
		// Do aging scheduling
		register struct	pentry	*optr;	/* pointer to old process entry */
		register struct	pentry	*nptr;	/* pointer to new process entry */

		optr = &proctab[currpid];

		// Add aging to all ready process
		int cur = q[rdyhead].qnext;
        while (cur != rdytail) { 
            int pid = cur;

            if (pid == NULLPROC) {
            	cur = q[cur].qnext;
                continue;
            }

            proctab[pid].aged_priority += AGING_INCREMENT;
            q[pid].qkey += AGING_INCREMENT;

            cur = q[cur].qnext;
        }

		// Check if there is process with higher priority
        if (optr->pstate == PRCURR && optr->aged_priority > lastkey(rdytail)) {
			// Priority of current process is still higher
            return OK;
        }

		if (isempty(rdyhead) && optr->pstate != PRCURR) {
			// If no other process is ready to run, run NULL process
			
			if (currpid == NULLPROC) {
                return OK; // NULL process is already running, do nothing
            }
			
			currpid = NULLPROC;
			nptr = &proctab[currpid];
			nptr->pstate = PRCURR;

			#ifdef RTCLOCK
			preempt = QUANTUM;
			#endif

			ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
			return OK;
		}

		// Get the process with highest priority and remove it from ready queue
		int new_pid = getlast(rdytail);

        if (optr->pstate == PRCURR) {
			/*
			Insert current process back to ready queue after we choose a new process, 
			in case there is a process with the same priority, so that round-robin
			is achieved.
			*/
            optr->pstate = PRREADY;
            insert(currpid, rdyhead, optr->pprio);
        }

		currpid = new_pid;
        nptr = &proctab[new_pid];
        nptr->pstate = PRCURR;
		
		// Reset the selected process's aged_priority to its base priority (pprio)
        nptr->aged_priority = nptr->pprio;

        #ifdef RTCLOCK
        preempt = QUANTUM;
        #endif

        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

        return OK;

	} else if (current_scheduling == LINUXSCHED) {
		register struct pentry *optr;
		register struct pentry *nptr;

		optr = &proctab[currpid];

		optr->quantum_left = preempt;
		if (optr->quantum_left <= 0) {
			optr->quantum_left = 0;
			optr->goodness = 0;
		} else {
			optr->goodness =  optr->pprio + optr->quantum_left;
		}

		// Check ready queue too see if new epoch is needed
		int has_runnable_process = 0;
		int cur = q[rdyhead].qnext;
		while (cur != rdytail) {
			int pid = cur;
			if (pid != NULLPROC && proctab[pid].quantum_left > 0) {
				// There's process that is still runnable, stay in the same epoch
				has_runnable_process = 1;
				break;
			}
			cur = q[cur].qnext;
		}
		// Check if current process is still capable to run.
		if (optr->pstate == PRCURR && currpid != NULLPROC && optr->quantum_left > 0) {
			// Since current process may still has quantum but is not in the ready queue
			has_runnable_process = 1;
		}
		
		if (!has_runnable_process) {
			next_epoch();

			// Reset has_runnable_process to check if Null process needs to run
			has_runnable_process = 0;
			cur = q[rdyhead].qnext;
			while (cur != rdytail) {
				int pid = cur;
				if (pid != NULLPROC && proctab[pid].quantum_left > 0) {
					has_runnable_process = 1;
					break;
				}
				cur = q[cur].qnext;
			}

			if (optr->pstate == PRCURR && currpid != NULLPROC && optr->quantum_left > 0) {
				// Since current process may still has quantum but is not in the ready queue
				has_runnable_process = 1;
			}

			if (!has_runnable_process) {

				if (currpid == NULLPROC) {
					// Current process is NULL process, and still no other process is ready to run and current process is NULLPROC
					return OK;
				}

				// No other process is ready to run, and current process is not runnable, switch to NULL process
				currpid = NULLPROC;
				nptr = &proctab[currpid];
				nptr->pstate = PRCURR;

				#ifdef RTCLOCK
				preempt = QUANTUM;
				#endif

				ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
				return OK;	
			}
		}

		// Check if there is process with higher priority
		if (optr->pstate == PRCURR && optr->goodness > lastkey(rdytail)) {
			// Priority of current process is still higher, continue to run
			preempt = optr->quantum_left;
			return OK;
		}

		int new_pid = getlast(rdytail);

        if (optr->pstate == PRCURR) {
			/*
			Insert current process back to ready queue after we choose a new process, 
			in case there is a process with the same priority, so that round-robin
			is achieved
			*/
            optr->pstate = PRREADY;
            insert(currpid, rdyhead, optr->goodness);
        }

		currpid = new_pid;
        nptr = &proctab[new_pid];
        nptr->pstate = PRCURR;

		#ifdef RTCLOCK
		preempt = nptr->quantum_left;
		#endif

		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
		return OK;
	} else {
		// Default XINU scheduling
		register struct	pentry	*optr;	/* pointer to old process entry */
		register struct	pentry	*nptr;	/* pointer to new process entry */

		/* no switch needed if current process priority higher than next*/

		if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
		(lastkey(rdytail)<optr->pprio)) {
			return(OK);
		}
		
		/* force context switch */

		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid,rdyhead,optr->pprio);
		}

		/* remove highest priority process at end of ready list */

		nptr = &proctab[ (currpid = getlast(rdytail)) ];
		nptr->pstate = PRCURR;		/* mark it currently running	*/
		#ifdef	RTCLOCK
		preempt = QUANTUM;		/* reset preemption counter	*/
		#endif
		
		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
		
		/* The OLD process returns here when resumed. */
		return OK;
	}
}
