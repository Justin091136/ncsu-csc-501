#include <stdio.h>

void printsegaddress()
{
    kprintf("void printsegaddress()\n");
    extern unsigned long edata, etext, end;

    kprintf("Current: ");
    kprintf("etext[0x%08x]=0x%08x", etext, *(unsigned long *)(etext));
    kprintf(", edata[0x%08x]=0x%08x", edata, *(unsigned long *)(edata));
    kprintf(", ebss[0x%08x]=0x%08x\n", end, *(unsigned long *)(end));

    kprintf("Preceding: ");
    kprintf("etext[0x%08x]=0x%08x", (etext - 4), *(unsigned long *)(etext - 4));
    kprintf(", edata[0x%08x]=0x%08x", (edata - 4), *(unsigned long *)(edata - 4));
    kprintf(", ebss[0x%08x]=0x%08x\n", (end - 4), *(unsigned long *)(end - 4));

    kprintf("After: ");
    kprintf("etext[0x%08x]=0x%08x", (etext + 4), *(unsigned long *)(etext + 4));
    kprintf(", edata[0x%08x]=0x%08x", (edata + 4), *(unsigned long *)(edata + 4));
    kprintf(", ebss[0x%08x]=0x%08x\n", (end + 4), *(unsigned long *)(end + 4));

    kprintf("\n");
}
