/*  releaseall.c  */
#include <stdio.h>
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lock.h"

void find_candidate(int lockid);
void assign_process_to_lock(int lockid, int procid);
void release_lock(int lockid, int pid);

int releaseall(int numlocks, int locks, ...) {
    STATWORD ps;
    disable(ps);

    int *lockaddr = (int *)(&locks);
    int sys_err = 0;
    int resched_needed = 0;


    int i;
    for (i = 0; i < numlocks; i++) {
        int lockid = *(lockaddr + i);
        
        if (!is_lock_id_valid(lockid)) {
            sys_err = 1;
            continue;
        }

        release_lock(lockid, currpid);
        resched_needed = 1;
    }

    if (resched_needed) {
        resched();
    }
    restore(ps);
    return (sys_err ? SYSERR : OK);
}

void release_lock(int lockid, int pid) {
    struct lockentry *lptr = &locktab[lockid];

    if (lptr->lstate == FREE || lptr->procs_hold_list[pid] != ACQUIRED) {
        return;
    }

    lptr->procs_hold_list[pid] = UNACQUIRED;
    proctab[pid].lock_list[lockid] = UNACQUIRED;

    if (lptr->ltype == READ) {
        lptr->reader_count--;
        if (lptr->reader_count > 0) {
            // Still has reader holding the lock
            return;
        }
    }

    if (isempty(lptr->lqhead)) {
        // No one wants to use the lock
        lptr->lprio = MININT;
        lptr->ltype = QUEUE_EMPTY;
        return;
    }

    find_candidate(lockid);

    update_inherited_priority(pid);

    propagate_priority_inheritance(lockid);
}

void find_candidate(int lockid) {
    struct lockentry *lptr = &locktab[lockid];

    int highest_priority = lastkey(lptr->lqtail);
    int procid = lastid(lptr->lqtail);
    int writer_pid = -1;  // Set to -1 for first time comparison
    int writer_priority = -1;
    int reader_pid = -1;  // Set to -1 for first time comparison
    unsigned long max_reader_wait_time = 0;
    unsigned long max_writer_wait_time = 0;
    int same_priority_n = 0;

    while (procid != lptr->lqhead) {
        if (q[procid].qkey == highest_priority) {
            same_priority_n++;

            if (proctab[procid].wait_type == WRITE) {
                unsigned long writer_wait_time = ctr1000 - proctab[procid].wait_time_start;
                if (writer_pid == -1 || writer_wait_time > max_writer_wait_time) {
                    writer_pid = procid;
                    writer_priority = q[procid].qkey;
                    max_writer_wait_time = writer_wait_time;
                }
            } else { // Reader
                unsigned long reader_wait_time = ctr1000 - proctab[procid].wait_time_start;
                if (reader_pid == -1 || reader_wait_time > max_reader_wait_time) {
                    reader_pid = procid;
                    max_reader_wait_time = reader_wait_time;
                }
            }
        }
        procid = q[procid].qprev;
    }

    if (same_priority_n == 1) {
        // Only 1 candidate with highest priority
        procid = lastid(lptr->lqtail);
    } else {
        // Multiple process with same priority
        if (writer_pid != -1 && reader_pid != -1) {
            // Writer and reader have same priority
            if (max_writer_wait_time >= max_reader_wait_time) {
                procid = writer_pid;
            } else {
                if (max_writer_wait_time + 500 >= max_reader_wait_time) {
                    // Grace period for writer
                    procid = writer_pid;
                } else {
                    procid = reader_pid;
                }
            }
        } else if (writer_pid != -1) {
            procid = writer_pid;
        } else if (reader_pid != -1) {
            procid = reader_pid;
        } else {            
            return;
        }
    }
    
    assign_process_to_lock(lockid, procid);

    if (lptr->ltype == READ) {
        int has_writer_waiting = 0;
        int highest_waiting_writer_priority = -1;
        int tmp_procid = lastid(lptr->lqtail);
        while (tmp_procid != lptr->lqhead) {
            if (proctab[tmp_procid].wait_type == WRITE) {
                has_writer_waiting = 1;
                highest_waiting_writer_priority = q[tmp_procid].qkey;
                break;
            }
            tmp_procid = q[tmp_procid].qprev;
        }
        int reader_list[NPROC];
        int reader_count = 0;

        if (has_writer_waiting) {
            // Need to wake up all reader with priority higher than writer
            tmp_procid = lastid(lptr->lqtail);;
            while (tmp_procid != lptr->lqhead) {
                if (proctab[tmp_procid].wait_type == READ && q[tmp_procid].qkey > highest_waiting_writer_priority) {
                    //kprintf("      Reader %d is blocking writer with priority%d\n", tmp_procid, q[tmp_procid].qkey);
                    reader_list[reader_count] = tmp_procid;
                    reader_count++;
                }
                tmp_procid = q[tmp_procid].qprev;
            }
        } else {
            // No writer is waiting, all the reader can join
            int tmp_procid = lastid(lptr->lqtail);
            while (tmp_procid != lptr->lqhead) {
                //kprintf("      No writer in queue, wake up reader %d\n", tmp_procid);
                reader_list[reader_count] = tmp_procid;
                reader_count++;            
                tmp_procid = q[tmp_procid].qprev;
            }
        }
    
        int i;
        for (i = 0; i < reader_count; i++) {
            assign_process_to_lock(lockid, reader_list[i]);
        }
    }
}


void assign_process_to_lock(int lockid, int procid) {
    if (procid == -1) return;
    struct lockentry *lptr = &locktab[lockid];
    struct pentry *pptr = &proctab[procid];

    dequeue(procid);
    lptr->ltype = pptr->wait_type;
    lptr->procs_hold_list[procid] = ACQUIRED;

    pptr->lock_return_val = OK;
    pptr->lock_list[lockid] = ACQUIRED;
    pptr->pstate = PRREADY;
    pptr->waiting_lock_id = -1;
    pptr->wait_type = -1;
    pptr->wait_time_start = 0;

    ready(procid, RESCHNO);
}



