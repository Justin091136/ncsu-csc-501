/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <q.h>
#include "lab1.h"

#define LOOP 50

int prA, prB, prC;
volatile int a_cnt = 0;
volatile int b_cnt = 0;
volatile int c_cnt = 0;
volatile int s = 0;

int proc_a(char c);
int proc_b(char c);
int proc_c(char c);

int proc_print(char c);

int proc_counter(char c);

int main()
{
    int i;
    int count = 0;
    char buf[8];

    srand(1234);

    kprintf("Please Input test case number:\n");
	
	kprintf("\n[Aging Scheduling]\n");
    kprintf("0: Original Aging Scheduling (proc_a, proc_b, proc_c)\n");
    kprintf("1: TEST1 (Aging, priorities=20, resume order: A, B, C)\n");
    kprintf("2: TEST2 (Aging, priorities=30, resume order: C, B, A)\n");
    kprintf("3: TEST3 (Aging, initial prio=10, then chprio to 30)\n");
    kprintf("4: TEST4 (Aging, prio: 10,9,10 then adjust B & current proc prio)\n");
	kprintf("\n[Linux-Like Scheduling]\n");
    kprintf("5: Original Linux Like Scheduling (proc_a, proc_b, proc_c)\n");
    kprintf("6: TEST1 (Linux Like, priorities=20, resume order: A, B, C)\n");
    kprintf("7: TEST2 (Linux Like, priorities=30, resume order: C, B, A)\n");
    kprintf("8: TEST3 (Linux Like, initial prio=10, then chprio to 30)\n");
    kprintf("9: TEST4 (Linux Like, prio: 10,9,10 then adjust B & current proc prio)\n");

    while ((i = read(CONSOLE, buf, sizeof(buf)-1)) < 1)
        ;
    buf[i] = '\0';
    s = atoi(buf);
    kprintf("Get %d\n", s);

    a_cnt = 0; b_cnt = 0; c_cnt = 0;

    switch(s) {
      case 0:
         kprintf("Original Aging Scheduling branch using proc_a, proc_b, proc_c\n");
         setschedclass(AGESCHED);
         prA = create(proc_a, 2000, 1, "proc A", 1, 'A');
         prB = create(proc_b, 2000, 2, "proc B", 1, 'B');
         prC = create(proc_c, 2000, 3, "proc C", 1, 'C');
         resume(prA);
         resume(prB);
         resume(prC);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
      case 1:
         kprintf("\n\n[Aging] TEST1:\n");
         setschedclass(AGESCHED);
         prA = create(proc_counter, 2000, 20, "proc A", 1, 'A');
         prB = create(proc_counter, 2000, 20, "proc B", 1, 'B');
         prC = create(proc_counter, 2000, 20, "proc C", 1, 'C');
         resume(prA);
         resume(prB);
         resume(prC);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
      case 2:
         kprintf("\n\n[Aging] TEST2:\n");
         setschedclass(AGESCHED);
         prA = create(proc_counter, 2000, 30, "proc A", 1, 'A');
         prB = create(proc_counter, 2000, 30, "proc B", 1, 'B');
         prC = create(proc_counter, 2000, 30, "proc C", 1, 'C');
         resume(prC);
         resume(prB);
         resume(prA);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
      case 3:
         kprintf("\n\n[Aging] TEST3:\n");
         setschedclass(AGESCHED);
         prA = create(proc_counter, 2000, 10, "proc A", 1, 'A');
         prB = create(proc_counter, 2000, 10, "proc B", 1, 'B');
         prC = create(proc_counter, 2000, 10, "proc C", 1, 'C');
         resume(prA);
         resume(prB);
         resume(prC);
         chprio(prA, 30);
         chprio(prB, 30);
         chprio(prC, 30);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
      case 4:
         kprintf("\n\n[Aging] TEST4:\n");
         setschedclass(AGESCHED);
         prA = create(proc_counter, 2000, 10, "proc A", 1, 'A');
         prB = create(proc_counter, 2000, 9, "proc B", 1, 'B');
         prC = create(proc_counter, 2000, 10, "proc C", 1, 'C');
         resume(prA);
         resume(prB);
         resume(prC);
         chprio(prB, 15);
         chprio(getpid(), 5);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
      case 5:
         kprintf("Using Linux Scheduling\n");
         setschedclass(LINUXSCHED);
         prA = create(proc_print, 2000, 5, "proc A", 1, 'A');
         prB = create(proc_print, 2000, 50, "proc B", 1, 'B');
         prC = create(proc_print, 2000, 90, "proc C", 1, 'C');
         resume(prA);
         resume(prB);
         resume(prC);
         while (count++ < LOOP)
         {
             kprintf("M");
             for (i = 0; i < 1000000; i++)
                 ;
         }
         kprintf("\n");
         break;
      case 6:
         kprintf("\n\n[Linux-Like] TEST1:\n");
         setschedclass(LINUXSCHED);
         prA = create(proc_counter, 2000, 20, "proc A", 1, 'A');
         prB = create(proc_counter, 2000, 20, "proc B", 1, 'B');
         prC = create(proc_counter, 2000, 20, "proc C", 1, 'C');
         resume(prA);
         resume(prB);
         resume(prC);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
      case 7:
         kprintf("\n\n[Linux-Like] TEST2:\n");
         setschedclass(LINUXSCHED);
         prA = create(proc_counter, 2000, 30, "proc A", 1, 'A');
         prB = create(proc_counter, 2000, 30, "proc B", 1, 'B');
         prC = create(proc_counter, 2000, 30, "proc C", 1, 'C');
         resume(prC);
         resume(prB);
         resume(prA);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
      case 8:
         kprintf("\n\n[Linux-Like] TEST3:\n");
         setschedclass(LINUXSCHED);
         prA = create(proc_counter, 2000, 10, "proc A", 1, 'A');
         prB = create(proc_counter, 2000, 10, "proc B", 1, 'B');
         prC = create(proc_counter, 2000, 10, "proc C", 1, 'C');
         resume(prA);
         resume(prB);
         resume(prC);
         chprio(prA, 30);
         chprio(prB, 30);
         chprio(prC, 30);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
      case 9:
         kprintf("\n\n[Linux-Like] TEST4:\n");
         setschedclass(LINUXSCHED);
         prA = create(proc_counter, 2000, 10, "proc A", 1, 'A');
         prB = create(proc_counter, 2000, 9, "proc B", 1, 'B');
         prC = create(proc_counter, 2000, 10, "proc C", 1, 'C');
         resume(prA);
         resume(prB);
         resume(prC);
         chprio(prB, 15);
         chprio(getpid(), 5);
         sleep(10);
         kill(prA);
         kill(prB);
         kill(prC);
         kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
         break;
		default:
			kprintf("Incorrect option\n");
			break;
    }
    return 0;
}

int proc_a(char c)
{
    int i;
    kprintf("Start... %c\n", c);
    b_cnt = 0;
    c_cnt = 0;
    while (1)
    {
        for (i = 0; i < 10000; i++)
            ;
        a_cnt++;
    }
    return 0;
}

int proc_b(char c)
{
    int i;
    kprintf("Start... %c\n", c);
    a_cnt = 0;
    c_cnt = 0;
    while (1)
    {
        for (i = 0; i < 10000; i++)
            ;
        b_cnt++;
    }
    return 0;
}

int proc_c(char c)
{
    int i;
    kprintf("Start... %c\n", c);
    a_cnt = 0;
    b_cnt = 0;
    while (1)
    {
        for (i = 0; i < 10000; i++)
            ;
        c_cnt++;
    }
    return 0;
}

int proc_print(char c)
{
    int i;
    int count = 0;
    while (count++ < LOOP)
    {
        kprintf("%c", c);
        for (i = 0; i < 1000000; i++)
            ;
    }
    return 0;
}

int proc_counter(char c)
{
    int i;
    while (1)
    {
        for (i = 0; i < 10000; i++)
            ;
        if (c == 'A')
            a_cnt++;
        if (c == 'B')
            b_cnt++;
        if (c == 'C')
            c_cnt++;
    }
    return 0;
}
