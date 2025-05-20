/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
#include <stdio.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  unsigned long fault_addr;
  asm("movl %%cr2, %0" : "=r"(fault_addr));
  
  // Convert address to page number
  unsigned int vpage = fault_addr >> 12;
  int pd_index = PAGE_DIRECTORY_INDEX(vpage);  //Page table index in page directory
  int pt_index = PAGE_TABLE_INDEX(vpage);  // Page index in page table

  int store, page_offset;
  if (bsm_lookup(currpid, fault_addr, &store, &page_offset) == SYSERR) {
    //kprintf("\n\n[pfint] bsm_lookup() failed: pid=%d, vpage=%d, store=%d , page_offset=%d \n", currpid, vpage, store, page_offset);
    //kprintf("Killing process %d\n", currpid);
    kill(currpid);
    return SYSERR;
  }

  pd_t *page_directory = (pd_t *)(proctab[currpid].pdbr);

  pd_t *pd_entry = &page_directory[pd_index];
  if (pd_entry->pd_pres == 0) {
    // Page table not exists
    int pt_frame;
    if (get_frm(&pt_frame) == SYSERR) {
        //kprintf("Failed to get_frm for the page table\n");
        return SYSERR;
    }

    //pt_t *new_table = (pt_t *)(pt_frame * NBPG);
    pt_t *new_table = (pt_t *)((FRAME0 + pt_frame) * NBPG);

    int i;
    for (i = 0; i < 1024; i++) {
      new_table[i].pt_pres   = 0;
      new_table[i].pt_write  = 1;
      new_table[i].pt_user   = 0;
      new_table[i].pt_pwt    = 0;
      new_table[i].pt_pcd    = 0;
      new_table[i].pt_acc    = 0;
      new_table[i].pt_dirty  = 0;
      new_table[i].pt_mbz    = 0;
      new_table[i].pt_global = 0;
      new_table[i].pt_avail  = 0;
      new_table[i].pt_base   = 0;
    }

    pd_entry->pd_pres = 1;
    pd_entry->pd_write = 1;
    pd_entry->pd_base = FRAME0 + pt_frame;

    // Update infomation for the frame that stores the page table
    frm_tab[pt_frame].fr_status = FRM_MAPPED;
    frm_tab[pt_frame].fr_pid = currpid;
    frm_tab[pt_frame].fr_type = FR_TBL;
    frm_tab[pt_frame].fr_refcnt = 0;
  }
  
  pt_t *page_table = (pt_t *)(NBPG * pd_entry->pd_base);
  pt_t *pt_entry = &page_table[pt_index];

  // Start to allocate new frame for the page
  if (pt_entry->pt_pres == 0) {
    int page_frame;
    if (get_frm(&page_frame) == SYSERR) {
        //kprintf("Failed to get_frm for the page\n");
        return SYSERR;
    }
    char *dest = (char *)((FRAME0 + page_frame) * NBPG);
    if (read_bs(dest, store, page_offset) == SYSERR) {
      //("[pfint] read_bs failed: store=%d, page=%d\n", store, page_offset);
      return SYSERR;
    }

    pt_entry->pt_pres = 1;
    pt_entry->pt_write = 1;
    pt_entry->pt_base = FRAME0 + page_frame;

    // Update infomation for the frame that stores the page
    frm_tab[page_frame].fr_status = FRM_MAPPED;
    frm_tab[page_frame].fr_pid = currpid;
    frm_tab[page_frame].fr_vpno = vpage;
    frm_tab[page_frame].fr_type = FR_PAGE;
    //frm_tab[page_frame].fr_dirty = 1;
    frm_tab[page_frame].ref_bit = 0;

    enqueue_frame(page_frame);  

    int pt_frame = pd_entry->pd_base - FRAME0;
    frm_tab[pt_frame].fr_refcnt++;
    invalidate_tlb_entry(fault_addr);
  }
  /*
  if (pt_entry->pt_pres == 1 && pt_entry->pt_dirty == 1) {
    frm_tab[pt_entry->pt_base - FRAME0].fr_dirty = 1;
  }
  */

  write_cr3(proctab[currpid].pdbr);
  return OK;
}


