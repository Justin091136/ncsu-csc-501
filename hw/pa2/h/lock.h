/*  lock.h  */
#ifndef _LOCK_H_
#define _LOCK_H_

#include <proc.h>

#define NLOCKS 50

// Lock type

#define QUEUE_EMPTY -1
#define READ 0
#define WRITE 1

// Lock status

#define USED 1  // Lock ID is used
#define FREE 0  // Lock ID is not created and cannot be used


#define ACQUIRED  1  // Lock is being held by some process
#define UNACQUIRED  0  // Lock is not being held


struct lockentry {
    int lstate;  // Free or Used
    int ltype;
    int lqhead;  // Head of the waiting queue
    int lqtail;  // Tail of the waiting queue
    int lprio; // Process with highest priority in the queue
    char procs_hold_list[NPROC]; // A table recording which process is holding the lock. 
    int reader_count;  // How many reader
};

extern struct lockentry locktab[NLOCKS];
extern int nextlock;
extern unsigned long ctr1000;

void linit();
int lcreate();
int ldelete(int lockid);
int lock(int lockid, int type, int priority);
int releaseall(int numlocks, int locks, ...);
void release_lock(int lockid, int pid);
int is_lock_id_valid(int lockid);
int get_process_priority(int pid);
void update_inherited_priority(int pid);
int get_max_prio_waiting_in_lock(int lockid);
void propagate_priority_inheritance(int lock_id);

#endif
