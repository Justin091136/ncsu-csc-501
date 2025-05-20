#include <stdio.h>
#include <kernel.h>
#include <proc.h>
#include "lab0.h"
#define MAX_STR_LENGTH 60

char *syscall_name[SYSCALL_FUNCTION_LIMIT] = {
    "freemem",
    "chprio",
    "getpid",
    "getprio",
    "gettime",
    "kill",
    "receive",
    "recvclr",
    "recvtim",
    "resume",
    "scount",
    "sdelete",
    "send",
    "setdev",
    "setnok",
    "screate",
    "signal",
    "signaln",
    "sleep",
    "sleep10",
    "sleep100",
    "sleep1000",
    "sreset",
    "stacktrace",
    "suspend",
    "unsleep",
    "wait"};

static int is_counting_syscall = 0;

// Record execution statistic for each syscall
static syscall_stat_t syscall_stat_table[NPROC][SYSCALL_FUNCTION_LIMIT];

extern volatile unsigned long ctr1000;

void _init_syscall_table()
{
    int pid;
    for (pid = 0; pid < NPROC; pid++)
    {
        int syscall_id;
        for (syscall_id = 0; syscall_id < SYSCALL_FUNCTION_LIMIT; syscall_id++)
        {
            syscall_stat_t *entry = &syscall_stat_table[pid][syscall_id];
            entry->count = 0;
            entry->total_exec_time = 0;
            entry->_last_start_time = 0;
        }
    }
}

void syscall_started(int process_id, int syscall_id)
{
    if (!is_counting_syscall)
    {
        return;
    }
    syscall_stat_t *entry = &syscall_stat_table[process_id][syscall_id];
    entry->_last_start_time = ctr1000;
}

void syscall_ended(int process_id, int syscall_id)
{
    if (!is_counting_syscall)
    {
        return;
    }
    syscall_stat_t *entry = &syscall_stat_table[process_id][syscall_id];
    entry->count += 1;

    entry->total_exec_time += (ctr1000 - entry->_last_start_time);
    entry->_last_start_time = 0;
}

void syscallsummary_start()
{
    _init_syscall_table();
    is_counting_syscall = 1;
}

void syscallsummary_stop()
{
    is_counting_syscall = 0;
}

void printsyscallsummary()
{
    kprintf("void printsyscallsummary()\n");
    extern struct pentry proctab[];

    int current_proc_index;
    for (current_proc_index = 0; current_proc_index < NPROC; current_proc_index++)
    {
        int has_printed_pid_header = 0;
        int current_syscall_id;
        for (current_syscall_id = 0; current_syscall_id < SYSCALL_FUNCTION_LIMIT; current_syscall_id++)
        {
            syscall_stat_t *entry = &syscall_stat_table[current_proc_index][current_syscall_id];
            if (entry->count > 0)
            {

                if (!has_printed_pid_header)
                {
                    kprintf("Process [pid:%d]\n", current_proc_index);
                    has_printed_pid_header = 1;
                }
                kprintf("    Syscall: [%s], count: %d, average execution time: %d (ms)\n", syscall_name[current_syscall_id], entry->count, entry->total_exec_time / entry->count);
            }
        }
        
    }
}
