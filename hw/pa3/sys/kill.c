/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:
		pptr->pstate = PRFREE;
		/* Paging cleanup */
		{
			int i, j;
			pd_t *page_directory = (pd_t *)proctab[pid].pdbr;

			for (i = 0; i < 1024; i++) {
				if (page_directory[i].pd_pres == 1) {
					unsigned int pt_frame = page_directory[i].pd_base - FRAME0;
					pt_t *page_table = (pt_t *)(page_directory[i].pd_base * NBPG);

					for (j = 0; j < 1024; j++) {
						if (page_table[j].pt_pres == 1) {
							int page_frame = page_table[j].pt_base - FRAME0;

							if (frm_tab[page_frame].fr_status == FRM_MAPPED &&
								frm_tab[page_frame].fr_type == FR_PAGE &&
								frm_tab[page_frame].fr_pid == pid) {

								free_frm(page_frame);
							}
						}
					}

					if (frm_tab[pt_frame].fr_status == FRM_MAPPED &&
						frm_tab[pt_frame].fr_type == FR_TBL &&
						frm_tab[pt_frame].fr_pid == pid) {

						free_frm(pt_frame);
					}
				}
			}

			int pd_frame = (proctab[pid].pdbr / NBPG) - FRAME0;
			if (frm_tab[pd_frame].fr_status == FRM_MAPPED &&
				frm_tab[pd_frame].fr_type == FR_DIR &&
				frm_tab[pd_frame].fr_pid == pid) {

				free_frm(pd_frame);
			}
		}
	}
	restore(ps);
	return(OK);
}
