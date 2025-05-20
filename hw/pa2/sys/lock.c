/*  lock.c  */
#include <stdio.h>
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lock.h"

int lock(int lockid, int lock_type, int lock_priority) {
    STATWORD ps;
    disable(ps);

    if (!is_lock_id_valid(lockid) || locktab[lockid].lstate == FREE || locktab[lockid].lstate == DELETED) {
        restore(ps);
        proctab[currpid].lock_return_val = SYSERR;
        return SYSERR;
    }

    struct lockentry *lptr = &locktab[lockid];

    if (lptr->ltype == QUEUE_EMPTY) {
        // Lock is available, grant access immediately
        lptr->ltype = lock_type;
        lptr->procs_hold_list[currpid] = ACQUIRED;
        proctab[currpid].lock_list[lockid] = ACQUIRED;
        proctab[currpid].lock_return_val = OK;

        if (lock_type == READ) {
            lptr->reader_count = 1;
        } else {
            lptr->reader_count = 0;
        }

        restore(ps);
        return OK;
    }

    int need_to_wait = 0;
    if (lptr->ltype == READ) { 
        if (lock_type == READ) {           
            // Check if there are any writer in the queue and has equal or higher priority 
            int procid = firstid(lptr->lqhead);
            while (procid != lptr->lqtail) {
                if (proctab[procid].wait_type == WRITE && q[procid].qkey >= lock_priority) {
                    // Higher priority writer exists, must wait
                    need_to_wait = 1;
                    break;
                }
                procid = q[procid].qnext;
            }
        } else {
            need_to_wait = 1; // Writer must always wait if readers exist
        }
    } else if (lptr->ltype == WRITE) {
        need_to_wait = 1;
    }

    if (need_to_wait) {

        // Put process in wait queue
        proctab[currpid].wait_type = lock_type;
        proctab[currpid].pstate = PRWAIT;
        proctab[currpid].waiting_lock_id = lockid;
        proctab[currpid].wait_time_start = ctr1000;
        insert(currpid, lptr->lqhead, lock_priority);
        
        // Priority Inheritance: Check if the lock holder needs to inherit priority
        int holder_pid;
        for (holder_pid = 0; holder_pid < NPROC; holder_pid++) {
            if (lptr->procs_hold_list[holder_pid] == ACQUIRED) {
                struct pentry *proc_ptr = &proctab[holder_pid];
    
                int old_priority = get_process_priority(holder_pid);
                update_inherited_priority(holder_pid);
                int new_priority = get_process_priority(holder_pid);
    
                if (old_priority != new_priority) {
                    if (proc_ptr->pstate == PRREADY) {
                        dequeue(holder_pid);
                        insert(holder_pid, rdyhead, get_process_priority(holder_pid));
                    }
                    
                    if (is_lock_id_valid(proc_ptr->waiting_lock_id)) {
                        propagate_priority_inheritance(proc_ptr->waiting_lock_id);
                    }
                }
            }
        }

        lptr->lprio = max(lptr->lprio, get_process_priority(currpid));

        resched();
        restore(ps);
        return proctab[currpid].lock_return_val;
    }

    // No longer need to wait, acquire lock immediately
    lptr->procs_hold_list[currpid] = ACQUIRED;
    proctab[currpid].lock_list[lockid] = ACQUIRED;
    proctab[currpid].lock_return_val = OK;
    proctab[currpid].waiting_lock_id = -1;
    proctab[currpid].wait_time_start = -1;
    proctab[currpid].wait_type = -1;

    if (lock_type == READ) {
        lptr->reader_count++;
    }

    restore(ps);
    return OK;
}

int is_lock_id_valid(int lockid) {
    if (lockid < 0 || lockid >= NLOCKS) {
        return 0;
    }
    return 1;
}

int get_process_priority(int pid) {
    return (proctab[pid].pinh == 0) ? proctab[pid].pprio : proctab[pid].pinh;
}

int get_max_prio_waiting_in_lock(int lockid) {
    // Process priority, not lock priority
    struct lockentry *lptr = &locktab[lockid];
    
    if (isempty(lptr->lqhead)) {
        return MININT;
    }

    int max_prio = MININT;
    int procid = lastid(lptr->lqtail);

    while (procid != lptr->lqhead) {  
        int pprio = get_process_priority(procid);  
        if (pprio > max_prio) {  
            max_prio = pprio;
        }
        procid = q[procid].qprev;
    }

    return max_prio;
}

void propagate_priority_inheritance(int lock_id) {
    if (!is_lock_id_valid(lock_id)) {
        return;
    }

    struct lockentry *lock_ptr = &locktab[lock_id];

    int pid;
    for (pid = 0; pid < NPROC; pid++) {
        if (lock_ptr->procs_hold_list[pid] == ACQUIRED) {
            struct pentry *proc_ptr = &proctab[pid];

            int old_priority = get_process_priority(pid);
            update_inherited_priority(pid);
            int new_priority = get_process_priority(pid);

            if (old_priority != new_priority) {
                if (proc_ptr->pstate == PRREADY) {
                    dequeue(pid);
                    insert(pid, rdyhead, get_process_priority(pid));
                }
                
                if (is_lock_id_valid(proc_ptr->waiting_lock_id)) {
                    propagate_priority_inheritance(proc_ptr->waiting_lock_id);
                }
            }

        }
    }
}

void update_inherited_priority(int pid) {
    struct pentry *pptr = &proctab[pid];

    int maxprio = MININT;
    int i;
    for (i = 0; i < NLOCKS; i++) {
        if (pptr->lock_list[i] == ACQUIRED) {
            int prio = get_max_prio_waiting_in_lock(i);
            if (prio > maxprio) {
                maxprio = prio;
            }
        }
    }
    pptr->pinh = (maxprio > pptr->pprio) ? maxprio : 0;
}
