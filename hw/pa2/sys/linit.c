/* linit.c */
#include <stdio.h>
#include <conf.h>
#include <proc.h>
#include <kernel.h>
#include <q.h>
#include "lock.h"

struct lockentry locktab[NLOCKS];
int nextlock;

void linit() {
    struct lockentry *lock_ptr;
	nextlock = 0;
	int lock_id;

	for (lock_id = 0; lock_id < NLOCKS; lock_id++) {
		lock_ptr = &locktab[lock_id];
		lock_ptr->lstate = FREE;
		lock_ptr->ltype = QUEUE_EMPTY;
		lock_ptr->lprio = MININT;

		lock_ptr->lqtail = 1 + (lock_ptr->lqhead = newqueue());

		int process_id;
		for (process_id = 0; process_id < NPROC; process_id++) {
			lock_ptr->procs_hold_list[process_id] = UNACQUIRED;
        }

        lock_ptr->reader_count = 0;
	}
}
