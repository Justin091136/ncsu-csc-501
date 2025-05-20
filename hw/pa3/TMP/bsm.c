/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bsm_tab[16];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    STATWORD ps;
    disable(ps);
    int i;
    /* Initialize all 16 backing stores*/
    for(i=0; i<16; i++)
    {
        bsm_tab[i].bs_status=BSM_UNMAPPED;
        bsm_tab[i].bs_pid=0;
        bsm_tab[i].bs_vpno=0;
        bsm_tab[i].bs_npages=0;
        bsm_tab[i].bs_sem=0;
        bsm_tab[i].bs_require_heap=0;
        bsm_tab[i].bs_cnt_proc=0;
        int j;
        for(j=0;j<NPROC;j++)
        {
            proctab[j].store=-1;
            proctab[j].vhpno=-1;
            proctab[j].vhpnpages=0;
        }
    }
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD ps;
    disable(ps);
    int i;
    for(i=0;i<16;i++)
    {
        /* if unmapped backing store found, map it and return*/
        if(bsm_tab[i].bs_status==BSM_UNMAPPED)
        {
            *avail=i;
            restore(ps);
            return OK;
        }
    }
    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    STATWORD ps;
    disable(ps);
    /* error cases */
    if(i<0 || i>=16 || bsm_tab[i].bs_status!=BSM_MAPPED)
    {
        restore(ps);
        return SYSERR;
    }
    bsm_tab[i].bs_status=BSM_UNMAPPED;
    bsm_tab[i].bs_require_heap=0;
    bsm_tab[i].bs_npages=0;
    bsm_tab[i].bs_cnt_proc=0;
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    STATWORD ps;
    disable(ps);
    int vpno = ((unsigned long)vaddr) >> 12;
    int i;
        
    for(i=0; i < 16; i++)
    {
        /* check if the backing store is mapped to provided pid */
        if(proctab[pid].store==i && vpno>=proctab[pid].vhpno && vpno<(proctab[pid].vhpno+proctab[pid].vhpnpages))
        {
            *store=i;
            *pageth=vpno-proctab[pid].vhpno;
            restore(ps);
            return OK;
        }
    }
    *store=-1;
    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD ps;
    disable(ps);
    /* saftery check before mapping */
    if(source<0 || source>=16 || npages<=0 || npages>128 || vpno<4096 || vpno>=1048575 || bsm_tab[source].bs_status==BSM_MAPPED && bsm_tab[source].bs_require_heap==1)
    {
        restore(ps);
        return SYSERR;
    }
    /* if backing store is unmapped map it */
    if(bsm_tab[source].bs_status==BSM_UNMAPPED)
    {
        bsm_tab[source].bs_status=BSM_MAPPED;
        bsm_tab[source].bs_npages=npages;
        bsm_tab[source].bs_cnt_proc++;
        proctab[pid].store=source;
        proctab[pid].vhpno=vpno;
        proctab[pid].vhpnpages=npages;
        restore(ps);
        return OK;
    }
    int i;
    for(i=0;i<NPROC;i++)
    {
        if(proctab[i].store==source && (vpno >= proctab[i].vhpno && vpno < (proctab[i].vhpno + proctab[i].vhpnpages) || (vpno+npages) >= proctab[i].vhpno && (vpno+npages) < (proctab[i].vhpno + proctab[i].vhpnpages) || vpno<proctab[i].vhpno && (vpno+npages) < (proctab[i].vhpno + proctab[i].vhpnpages)))
        {
            //kprintf("\nerror to map bsm\n");
            restore(ps);
            return SYSERR;
        }
    }
    bsm_tab[source].bs_cnt_proc++;
    bsm_tab[source].bs_npages+=npages;
    proctab[pid].store=source;
    proctab[pid].vhpno=vpno;
    proctab[pid].vhpnpages=npages;
    restore(ps);
    return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    STATWORD ps;
    disable(ps);
    if(vpno<4096 || vpno>=1048575)
    {
        restore(ps);
        return SYSERR;
    }
    int i;
    for(i=0;i<16;i++)
    {
        if(bsm_tab[i].bs_status==BSM_UNMAPPED || proctab[pid].store!=i)
        {
            continue;
        }
        /* free store and frames associated with the store */
        if(proctab[pid].store==i && vpno>=proctab[pid].vhpno && vpno<(proctab[pid].vhpno+proctab[pid].vhpnpages))
        {
            bsm_tab[i].bs_npages-=proctab[pid].vhpnpages;
            proctab[pid].store=-1;
            proctab[pid].vhpno=-1;
            proctab[pid].vhpnpages=0;
            bsm_tab[i].bs_cnt_proc--;
            if(bsm_tab[i].bs_require_heap==1 || bsm_tab[i].bs_cnt_proc==0)
            {
                free_bsm(i);
            }
        }
    }
    restore(ps);
    return OK;
}

int set_private_heap(int source, int pid) {
    if (bsm_tab[source].bs_status==BSM_UNMAPPED || (bsm_tab[source].bs_require_heap == 1)) {
        return SYSERR;
    }
    /* set the flag of backing store */
    bsm_tab[source].bs_require_heap = 1;
    return 1;
}

void allocate_heap_to_backingstore(int bsm_id, int	hsize) {
    struct mblock *bs_mblock;
    bs_mblock = BACKING_STORE_BASE + (bsm_id * BACKING_STORE_UNIT_SIZE);
	bs_mblock->mlen = hsize * NBPG;
	bs_mblock->mnext = NULL;
}