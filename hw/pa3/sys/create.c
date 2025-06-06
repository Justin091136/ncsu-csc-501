/* create.c - create, newpid */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

LOCAL int newpid();

/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL create(procaddr,ssize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	unsigned long	savsp, *pushsp;
	STATWORD 	ps;    
	int		pid;		/* stores new process id	*/
	struct	pentry	*pptr;		/* pointer to proc. table entry */
	int		i;
	unsigned long	*a;		/* points to list of args	*/
	unsigned long	*saddr;		/* stack address		*/
	int		INITRET();

	disable(ps);
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (int) roundew(ssize);
	if (((saddr = (unsigned long *)getstk(ssize)) ==
	    (unsigned long *)SYSERR ) ||
	    (pid=newpid()) == SYSERR || priority < 1 || setup_page_dir(pid) == SYSERR) {
		restore(ps);
		return(SYSERR);
	}

	numproc++;
	pptr = &proctab[pid];

	pptr->fildes[0] = 0;	/* stdin set to console */
	pptr->fildes[1] = 0;	/* stdout set to console */
	pptr->fildes[2] = 0;	/* stderr set to console */

	for (i=3; i < _NFILE; i++)	/* others set to unused */
		pptr->fildes[i] = FDFREE;

	pptr->pstate = PRSUSP;
	for (i=0 ; i<PNMLEN && (int)(pptr->pname[i]=name[i])!=0 ; i++)
		;
	pptr->pprio = priority;
	pptr->pbase = (long) saddr;
	pptr->pstklen = ssize;
	pptr->psem = 0;
	pptr->phasmsg = FALSE;
	pptr->plimit = pptr->pbase - ssize + sizeof (long);	
	pptr->pirmask[0] = 0;
	pptr->pnxtkin = BADPID;
	pptr->pdevs[0] = pptr->pdevs[1] = pptr->ppagedev = BADDEV;

		/* Bottom of stack */
	*saddr = MAGIC;
	savsp = (unsigned long)saddr;

	/* push arguments */
	pptr->pargs = nargs;
	a = (unsigned long *)(&args) + (nargs-1); /* last argument	*/
	for ( ; nargs > 0 ; nargs--)	/* machine dependent; copy args	*/
		*--saddr = *a--;	/* onto created process' stack	*/
	*--saddr = (long)INITRET;	/* push on return address	*/

	*--saddr = pptr->paddr = (long)procaddr; /* where we "ret" to	*/
	*--saddr = savsp;		/* fake frame ptr for procaddr	*/
	savsp = (unsigned long) saddr;

/* this must match what ctxsw expects: flags, regs, old SP */
/* emulate 386 "pushal" instruction */
	*--saddr = 0;
	*--saddr = 0;	/* %eax */
	*--saddr = 0;	/* %ecx */
	*--saddr = 0;	/* %edx */
	*--saddr = 0;	/* %ebx */
	*--saddr = 0;	/* %esp; fill in below */
	pushsp = saddr;
	*--saddr = savsp;	/* %ebp */
	*--saddr = 0;		/* %esi */
	*--saddr = 0;		/* %edi */
	*pushsp = pptr->pesp = (unsigned long)saddr;

	restore(ps);

	return(pid);
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL int newpid()
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

/*
int setup_page_dir(int pid) {
	int fr_id = 0;
	pd_t *page_directory;
	if(get_frm(&fr_id) == SYSERR) {
		return SYSERR;
	}
	proctab[pid].pdbr = (FRAME0 + fr_id) * NBPG;
	frm_tab[fr_id].fr_pid = pid;
	frm_tab[fr_id].fr_type = FR_DIR;
	frm_tab[fr_id].fr_status = FRM_MAPPED;
	page_directory = (pd_t *)proctab[pid].pdbr;
	int i;
	for (i = 0; i < 1024; i++) {
		page_directory[i].pd_pres = 0;  // default not present
		page_directory[i].pd_write = 1;
		page_directory[i].pd_base = 0;
	}
	for (i = 0; i < NUM_GLOBAL_PAGE_TABLES; i++) {
		page_directory[i].pd_pres = 1;
		page_directory[i].pd_write = 1;
		page_directory[i].pd_base = FRAME0 + i;  // Use shared PTs
		//page_directory[i].pd_global = 1; 
	}
	return OK;
}
*/


int setup_page_dir(int pid) {
    int fr_id;
    pd_t *page_directory;

    // Allocate a frame for this process's page directory
    if (get_frm(&fr_id) == SYSERR) {
        return SYSERR;
    }

    proctab[pid].pdbr = (FRAME0 + fr_id) * NBPG;
    frm_tab[fr_id].fr_pid = pid;
    frm_tab[fr_id].fr_type = FR_DIR;
    frm_tab[fr_id].fr_status = FRM_MAPPED;

    page_directory = (pd_t *)proctab[pid].pdbr;

    // Initialize all 1024 PDEs as not present
	int i;
    for (i = 0; i < 1024; i++) {
        page_directory[i].pd_pres = 0;
        page_directory[i].pd_write = 1;
        page_directory[i].pd_user = 0;
        page_directory[i].pd_pwt = 0;
        page_directory[i].pd_pcd = 0;
        page_directory[i].pd_acc = 0;
        page_directory[i].pd_mbz = 0;
        page_directory[i].pd_fmb = 0;
        page_directory[i].pd_global = 1;  // This field has no effect but reset for clarity
        page_directory[i].pd_avail = 0;
        page_directory[i].pd_base = 0;
    }

    // Hook up the first 4 global page tables (identity-mapped 0–16MB)
    for (i = 0; i < NUM_GLOBAL_PAGE_TABLES; i++) {
        page_directory[i].pd_pres = 1;
        page_directory[i].pd_write = 1;
        page_directory[i].pd_user = 0;
        page_directory[i].pd_pwt = 0;
        page_directory[i].pd_pcd = 0;
        page_directory[i].pd_acc = 0;
        page_directory[i].pd_mbz = 0;
        page_directory[i].pd_fmb = 0;
        page_directory[i].pd_global = 1;  // TLB won't flush global pages on CR3 write
        page_directory[i].pd_avail = 0;
        page_directory[i].pd_base = FRAME0 + i;
    }

    return OK;
}
