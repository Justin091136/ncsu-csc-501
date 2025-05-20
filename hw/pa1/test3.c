#include <conf.h>
#include <kernel.h>
#include <q.h>
#include <lab1.h>

#define LOOP 50

int prA, prB, prC;
int proc(char);
volatile int a_cnt = 0;
volatile int b_cnt = 0;
volatile int c_cnt = 0;

int main()
{
	kprintf("\n\nTEST3:\n");

	setschedclass(AGESCHED);

	prA = create(proc, 2000, 10, "proc A", 1, 'A');
	prB = create(proc, 2000, 10, "proc B", 1, 'B');
	prC = create(proc, 2000, 10, "proc C", 1, 'C');

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
	return 0;
}

int proc(char c)
{
	int i;
	while (1)
	{
		for (i = 0; i < 10000; i++);
		if (c == 'A') a_cnt++;
		if (c == 'B') b_cnt++;
		if (c == 'C') c_cnt++;
	}
	return 0;
}
