#ifndef LAB0_H
#define LAB0_H

#define SYSCALL_FUNCTION_LIMIT 43

long zfunction(long param);
void printsegaddress(void);
void printtos(void);
void printprocstks(int priority);
void printsyscallsummary(void);
void syscall_started(int process_id, int syscall_id);
void syscall_ended(int process_id, int syscall_id);
void syscallsummary_start(void);
void syscallsummary_stop(void);

typedef struct syscall_stat
{
    int count;
    long total_exec_time;
    long _last_start_time;
} syscall_stat_t;

typedef enum
{
    SYSCALL_FREEMEM,
    SYSCALL_CHPRIO,
    SYSCALL_GETPID,
    SYSCALL_GETPRIO,
    SYSCALL_GETTIME,
    SYSCALL_KILL,
    SYSCALL_RECEIVE,
    SYSCALL_RECVCLR,
    SYSCALL_RECVTIM,
    SYSCALL_RESUME,
    SYSCALL_SCOUNT,
    SYSCALL_SDELETE,
    SYSCALL_SEND,
    SYSCALL_SETDEV,
    SYSCALL_SETNOK,
    SYSCALL_SCREATE,
    SYSCALL_SIGNAL,
    SYSCALL_SIGNALN,
    SYSCALL_SLEEP,
    SYSCALL_SLEEP10,
    SYSCALL_SLEEP100,
    SYSCALL_SLEEP1000,
    SYSCALL_SRESET,
    SYSCALL_STACKTRACE,
    SYSCALL_SUSPEND,
    SYSCALL_UNSLEEP,
    SYSCALL_WAIT
} syscall_id_t;

#endif