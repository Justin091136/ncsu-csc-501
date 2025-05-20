/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include "lab0.h"

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

void test_zfunction()
{
	long inputs[] = {
		0xAABBCCDD, // 0xbbc00dff
		0x12345678, // 0x345008FF
		0xF1F2F3F4,
		0xAAAAAAAA};

	int i;
	for (i = 0; i < 3; i++)
	{
		long result = zfunction(inputs[i]);
		kprintf("Input:  0x%08lx, Result: 0x%08lx\n", inputs[i], result);
	}
}

void test_printsegaddress()
{
	printsegaddress();
}

void test_printtos()
{
	int i = 45;
	printtos();
}

void dummy_process()
{
	int sleep_time = (rand() % 6) + 2;
	//kprintf("Process %d sleeping for %d seconds\n", currpid, sleep_time);
	sleep(sleep_time);
	//kprintf("Process %d exiting\n", currpid);
}

void timed_process(int duration) {
	extern	unsigned long clktime;
    unsigned long start_time = clktime;
	volatile int i;
    //kprintf("Process started. Running for %d seconds.\n", duration);

    while (clktime - start_time < duration) {
        for (i = 0; i < 1000000; i++);
    }

    //kprintf("Process completed after %d seconds.\n", duration);
}

void test_printprocstks()
{
	resume(create(dummy_process, 1024, 7, "Process1", 1, 0));
	resume(create(dummy_process, 1024, 10, "Process2", 1, 0));
	resume(create(dummy_process, 1024, 13, "Process3", 1, 0));
	resume(create(timed_process, 1024, 15, "TimedProcess", 1, 10));
	//kprintf("Priority of main(): %d\n", proctab[currpid].pprio);

	int testing_priority = 10;
	printprocstks(testing_priority);
}

void sleep_5_secons(char c) {
	int i;
    sleep(5);

	int current_pid = getpid();
	getprio(current_pid);
	getprio(current_pid);
	getprio(current_pid);
}

void test_printsyscallsummary()
{
    syscallsummary_start();
	int current_pid = getpid();
    int new_process_pid = create(sleep_5_secons, 2000, 20, "proc X", 1, 'A');
    resume(new_process_pid);
    sleep(7);  // Wait for child process to complete

	sleep(1);

	current_pid = getpid();

	getprio(current_pid);

	sleep(1);

	getprio(current_pid);

	current_pid = getpid();

	sleep(1);

	getprio(current_pid);
	getprio(current_pid);

    syscallsummary_stop();
    printsyscallsummary();
}

void test_lab0_function()
{
	test_zfunction();
	test_printsegaddress();
	test_printtos();
	test_printprocstks();
	test_printsyscallsummary();
}

int main()
{
	int is_deubg = 1;
	kprintf("\n\nHello World, Xinu lives\n\n");
	if (is_deubg)
	{
		test_lab0_function();
	}
	return 0;
}
