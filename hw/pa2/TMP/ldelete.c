/*  ldelete.c  */
#include <stdio.h>
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lock.h"

int ldelete(int lockid) {
    STATWORD ps;
    disable(ps);

    if (!is_lock_id_valid(lockid) || locktab[lockid].lstate == FREE) {
        restore(ps);
        return SYSERR;
    }

    if (locktab[lockid].lstate == DELETED) {
        restore(ps);
        return SYSERR;
    }
    
    struct lockentry *lock_ptr = &locktab[lockid];

    // Mark the lock as free and reset its attributes
    lock_ptr->lstate = FREE;
    lock_ptr->ltype = QUEUE_EMPTY;
    lock_ptr->lprio = MININT;
    lock_ptr->reader_count = 0;

    // Reset the mappings for processes holding the lock
    int pid;
    for (pid = 0; pid < NPROC; pid++) {
        if (lock_ptr->procs_hold_list[pid] == ACQUIRED) {
            lock_ptr->procs_hold_list[pid] = UNACQUIRED;
            proctab[pid].lock_list[lockid] = UNACQUIRED;
            proctab[pid].lock_return_val = DELETED;
            update_inherited_priority(pid);

            if (proctab[pid].pinh > 0) {
                int waiting_lock = proctab[pid].waiting_lock_id;
                if (is_lock_id_valid(waiting_lock)) {
                    propagate_priority_inheritance(waiting_lock);
                }
            }
        }
    }

    // Move all waiting processes to the ready queue and mark their return value as deleted
    if (nonempty(lock_ptr->lqhead)) {
        int waiting_pid;
        while ((waiting_pid = getfirst(lock_ptr->lqhead)) != EMPTY) {
            proctab[waiting_pid].lock_return_val = DELETED;
            proctab[waiting_pid].waiting_lock_id = -1;
            ready(waiting_pid, RESCHNO);
        }

        resched(); // Reschedule after waking up all waiting processes
    }

    restore(ps);
    return OK;
}
