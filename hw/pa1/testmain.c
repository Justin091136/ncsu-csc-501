#include <conf.h>
#include <kernel.h>
#include <q.h>
#include <stdio.h>
#include <lab1.h>

#define LOOP 50

int prA, prB, prC;
int proc_a(char), proc_b(char), proc_c(char);
int proc(char);
int proc_test(char);
void run_test1();
void run_test2();
void run_test3();
void run_test4();

volatile int a_cnt = 0;
volatile int b_cnt = 0;
volatile int c_cnt = 0;
volatile int s = 0;

int main()
{
    int i;
    int count = 0;
    char buf[8];

    srand(1234);

    kprintf("Select Test Case:\n");
    kprintf("1: Original Aging Scheduling\n");
    kprintf("2: Original Linux-like Scheduling\n");
    kprintf("3: Test Case 1 (Equal priority)\n");
    kprintf("4: Test Case 2 (Resume order test)\n");
    kprintf("5: Test Case 3 (Priority change mid-epoch)\n");
    kprintf("6: Test Case 4 (Priority change & chprio())\n");

    while ((i = read(CONSOLE, buf, sizeof(buf))) < 1);
    buf[i] = 0;
    s = atoi(buf);
    kprintf("Running Test %d\n", s);

    switch (s) {
        case 1:
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
        
        case 2:
            setschedclass(LINUXSCHED);
            resume(prA = create(proc, 2000, 5, "proc A", 1, 'A'));
            resume(prB = create(proc, 2000, 50, "proc B", 1, 'B'));
            resume(prC = create(proc, 2000, 90, "proc C", 1, 'C'));

            while (count++ < LOOP) {
                kprintf("M");
                for (i = 0; i < 1000000; i++);
            }
            kprintf("\n");
            break;

        case 3:
            run_test1();
            break;

        case 4:
            run_test2();
            break;

        case 5:
            run_test3();
            break;

        case 6:
            run_test4();
            break;

        default:
            kprintf("Invalid option. Exiting.\n");
            break;
    }

    return 0;
}

// **Test Case 1**
void run_test1() {
    kprintf("\n\nRunning Test1:\n");

    setschedclass(AGESCHED);

    prA = create(proc_test, 2000, 20, "proc A", 1, 'A');
    prB = create(proc_test, 2000, 20, "proc B", 1, 'B');
    prC = create(proc_test, 2000, 20, "proc C", 1, 'C');

    resume(prA);
    resume(prB);
    resume(prC);

    sleep(10);

    kill(prA);
    kill(prB);
    kill(prC);

    kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
}

// **Test Case 2**
void run_test2() {
    kprintf("\n\nRunning Test2:\n");

    setschedclass(AGESCHED);

    prA = create(proc_test, 2000, 30, "proc A", 1, 'A');
    prB = create(proc_test, 2000, 30, "proc B", 1, 'B');
    prC = create(proc_test, 2000, 30, "proc C", 1, 'C');

    resume(prC);
    resume(prB);
    resume(prA);

    sleep(10);

    kill(prA);
    kill(prB);
    kill(prC);

    kprintf("\nTest Result: A = %d, B = %d, C = %d\n", a_cnt, b_cnt, c_cnt);
}

// **Test Case 3**
void run_test3() {
    kprintf("\n\nRunning Test3:\n");

    setschedclass(AGESCHED);

    prA = create(proc_test, 2000, 10, "proc A", 1, 'A');
    prB = create(proc_test, 2000, 10, "proc B", 1, 'B');
    prC = create(proc_test, 2000, 10, "proc C", 1, 'C');

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
}

// **Test Case 4**
void run_test4() {
    kprintf("\n\nRunning Test4:\n");

    setschedclass(AGESCHED);

    prA = create(proc_test, 2000, 10, "proc A", 1, 'A');
    prB = create(proc_test, 2000, 9, "proc B", 1, 'B');
    prC = create(proc_test, 2000, 10, "proc C", 1, 'C');

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
}

// **General Process Function**
int proc_test(char c) {
    int i;
    while (1) {
        for (i = 0; i < 10000; i++);
        if (c == 'A') a_cnt++;
        if (c == 'B') b_cnt++;
        if (c == 'C') c_cnt++;
    }
    return 0;
}

// **General Process for Linux-like Scheduling**
int proc(char c) {
    int i;
    int count = 0;

    while (count++ < LOOP) {
        kprintf("%c", c);
        for (i = 0; i < 1000000; i++);
    }
    return 0;
}
