/*  lcreate.c  */
#include <stdio.h>
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lock.h"

int lcreate() {
    STATWORD ps;
    disable(ps);

    int i;
    for (i = 0; i < NLOCKS; i++) {
        int lockid = nextlock;
        nextlock = (nextlock + 1) % NLOCKS;

        if (locktab[lockid].lstate == FREE) {
            locktab[lockid].lstate = USED;
            locktab[lockid].ltype = QUEUE_EMPTY;
            locktab[lockid].lprio = MININT; 
            locktab[lockid].reader_count = 0;

            int pid;
            for (pid = 0; pid < NPROC; pid++) {
                locktab[lockid].procs_hold_list[pid] = UNACQUIRED;
            }
            restore(ps);
            return lockid;
        }
    }
    restore(ps);
    return SYSERR;
}
