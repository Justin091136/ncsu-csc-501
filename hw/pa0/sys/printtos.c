#include <stdio.h>

#define MAX_STACK_DEPTH 4
/*
Output Example:
    void printtos()
    Before[0x00000000]: 0x00000000
    After [0x00111111]: 0x00ffffff
        element[0x00ffffff]: 0x00000003
        element[0x00ffffff]: 0x00000002
        element[0x00ffffff]: 0x00000001
        element[0x00ffffff]: 0x00000000
*/

unsigned long ebp;
unsigned long esp;

void printtos()
{
    asm("movl %%ebp, %0" : "=r"(ebp));
    asm("movl %%esp, %0" : "=r"(esp));
    kprintf("void printtos()\n");

    int i = 0;
    // Since there's no parameter for this function, the previous esp is ebp + 8, while esp + 4 is the return address to the previous calling function
    kprintf("Before[0x%08x]: 0x%08x\n", (ebp + 8), *(unsigned long*)(ebp + 8));
    kprintf("After [0x%08x]: 0x%08x\n", esp, *(unsigned long*)esp);

    unsigned long current_pointer =  esp + 4; // The first stack element "below" top of the stack

    while ((i < MAX_STACK_DEPTH) && (current_pointer < ebp)) {
        kprintf("    element %d[0x%08x]: 0x%08x\n", i+1, current_pointer, *(unsigned long*)current_pointer);
        current_pointer += 4;
        i++;
    }
    kprintf("\n");
}
