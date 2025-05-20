/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
 	disable(ps);
 	int bsm_id;
	struct mblock *initial_block;

	if (hsize < 0) {
		//kprintf("\nInvalid heap size");
		restore(ps);
		return SYSERR;
	}

	/* create normal process */
 	int pid = create(procaddr, ssize, priority, name, nargs, args);
 	if (pid == SYSERR) {
 		//kprintf("Failed to create process");
 		restore(ps);
 		return SYSERR;
 	}

 	if (get_bsm(&bsm_id) == SYSERR) {
 		//kprintf("Failed to get backing store for private heap");
		kill(pid);
 		restore(ps);
 		return SYSERR;
 	}

	int result = bsm_map(pid, 4096, bsm_id, hsize);
	if (result == SYSERR) {
		free_bsm(bsm_id); 
		proctab[pid].pstate = PRFREE;
		kill(pid);
		restore(ps);
		return SYSERR;
	}

	/* set private heap */
	set_private_heap(bsm_id, pid);
	
	/* allocate heap to backing store */
	allocate_heap_to_backingstore(bsm_id, hsize);

	proctab[pid].vmemlist->mnext = (struct mblock*)(4096 * NBPG);
	proctab[pid].vmemlist->mlen = 0; 

 	restore(ps);
	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
