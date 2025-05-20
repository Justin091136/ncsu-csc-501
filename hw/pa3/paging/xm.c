/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  //kprintf("xmmap - to be implemented!\n");
  STATWORD ps;
  disable(ps);
  if(source<0 || source>=16 || virtpage<4096 || virtpage>=1048575 || npages<=0 || npages>128 || bsm_tab[source].bs_status==BSM_MAPPED && (bsm_tab[source].bs_require_heap==1 || npages>(128-bsm_tab[source].bs_npages)))
  {
    restore(ps);
    return SYSERR;
  }
  bsm_map(currpid, virtpage, source, npages);
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
    STATWORD ps;
    disable(ps);

    /* Valid range check (only user space) */
    if (virtpage < 4096 || virtpage >= 1048575) {
        restore(ps);
        return SYSERR;
    }

    /* Retrieve the process's backing store and number of pages */
    int store  = proctab[currpid].store;
    int vpbase = proctab[currpid].vhpno;       /* Starting virtual page number */
    int npages = proctab[currpid].vhpnpages;

    if (store == -1 || virtpage != vpbase) {   /* Not a valid starting point */
        restore(ps);
        return SYSERR;
    }

    /* Process each page: if dirty → write back → clear PTE → free frame */
    pd_t *pgdir = (pd_t *)proctab[currpid].pdbr;

    int i;
    for (i = 0; i < npages; i++) {
        unsigned vpno   = vpbase + i;
        unsigned long vaddr = vpno << 12;

        /* Locate the corresponding page-table entry */
        int pd_index = PAGE_DIRECTORY_INDEX(vpno);
        int pt_index = PAGE_TABLE_INDEX(vpno);

        pd_t *pd_entry = &pgdir[pd_index];
        if (pd_entry->pd_pres == 0)            /* No page table → skip */
            continue;

        pt_t *pt_base  = (pt_t *)(pd_entry->pd_base * NBPG);
        pt_t *pt_entry = &pt_base[pt_index];
        if (pt_entry->pt_pres == 0)            /* Page not in RAM */
            continue;

        int frm = pt_entry->pt_base - FRAME0;  /* Get frame index */

        /* If dirty bit is set, write the page back to the backing store */
        if (pt_entry->pt_dirty) {
            write_bs((char *)((FRAME0 + frm) * NBPG), store, i);
        }

        /* Clear PTE, invalidate TLB, and free the frame */
        pt_entry->pt_pres  = 0;
        pt_entry->pt_dirty = 0;
        invalidate_tlb_entry(vaddr);           /* Existing helper function */
        free_frm(frm);                         /* Your implemented frame release function */
    }

    /* Update bsm / proctab tables */
    bsm_unmap(currpid, virtpage, 0);

    restore(ps);
    return OK;
}