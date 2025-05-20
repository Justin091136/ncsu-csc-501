#include <stdio.h>
#include <kernel.h>
#include <proc.h>

/*
void printprocstks(int priority)
Process [proc A]
    pid: 11
    priority: 40
    base: 0x00ff0ff0
    limit: 0x00ffffff
    len: 1024
    pointer: 0x 00ffff22
*/

void print_process_state(char state) {
    switch (state) {
        case PRCURR:
            printf("    Status: PRCURR\n");
            break;
        case PRFREE:
            printf("    Status: PRFREE\n");
            break;
        case PRREADY:
            printf("    Status: PRREADY\n");
            break;
        case PRRECV:
            printf("    Status: PRRECV\n");
            break;
        case PRSLEEP:
            printf("    Status: PRSLEEP\n");
            break;
        case PRSUSP:
            printf("    Status: PRSUSP\n");
            break;
        case PRWAIT:
            printf("    Status: PRWAIT\n");
            break;
        case PRTRECV:
            printf("    Status: PRTRECV\n");
            break;
        default:
            printf("    Status: Unknown\n");
            break;
    }
}

void printprocstks(int priority)
{
    kprintf("void printprocstks(int priority)\n");

    extern struct pentry proctab[];
    struct pentry *current_proc;

    int current_proc_index;
    for (current_proc_index = 0; current_proc_index < NPROC; current_proc_index++)
    {
        current_proc = &(proctab[current_proc_index]);
        if (current_proc->pstate != PRFREE)
        {
            if (current_proc->pprio > priority)
            {
                kprintf("Process [%s]\n", current_proc->pname);
                kprintf("    pid: %d\n", current_proc_index);
                kprintf("    priority: %d\n", current_proc->pprio);
                kprintf("    base: 0x%08x\n", current_proc->pbase);
                kprintf("    limit: 0x%08x\n", current_proc->plimit);
                kprintf("    len: %d\n", current_proc->pstklen);

                unsigned long current_stack_pointer;

                if (current_proc->pstate == PRCURR)
                {
                    //print_process_state(current_proc->pstate);
                    asm("movl %%esp, %0" : "=r"(current_stack_pointer));
                }
                else
                {
                    //print_process_state(current_proc->pstate);
                    current_stack_pointer = (unsigned long )current_proc->pesp;
                }

                kprintf("    pointer: 0x%08x\n", current_stack_pointer);
            }
        }
    }
    kprintf("\n");
}
