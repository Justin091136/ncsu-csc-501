/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[NFRAMES];

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  // frm_tab[0] starts from frame0 (1024)
  int i;
  for (i = 0; i < NFRAMES; i++) {
      frm_tab[i].fr_status = FRM_UNMAPPED;
      frm_tab[i].fr_pid = -1;
      frm_tab[i].fr_vpno = -1;
      frm_tab[i].fr_refcnt = 0;
      frm_tab[i].fr_type = -1;
      frm_tab[i].fr_dirty = 0;
      frm_tab[i].ref_bit = 0;
  }
  return OK;
}

void invalidate_tlb_entry(unsigned long vaddr)
{
    asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

// Mark the given virtual page as not present in the page table
void invalidate_page(int pid, int vpage)
{
    // Get process's page directory
    pd_t *page_directory = (pd_t *)(proctab[pid].pdbr);

    // Convert vpage into page directory index and page table index
    int pd_index = PAGE_DIRECTORY_INDEX(vpage);
    int pt_index = PAGE_TABLE_INDEX(vpage);

    pd_t *pd_entry = &page_directory[pd_index];
    if (pd_entry->pd_pres == 0) {
        return;
    }

    // Covert pd_entry->pd_base from frame number to memory address by multiplying NBPG(4KB)
    pt_t *page_table = (pt_t *)(pd_entry->pd_base * NBPG);

    page_table[pt_index].pt_pres = 0;
    page_table[pt_index].pt_dirty = 0;

    // Clear TLB cache
    if (pid == currpid) {
      invalidate_tlb_entry(vpage * NBPG);
    }

    // If page table is empty, remove it from memory
    int pt_frame = pd_entry->pd_base - FRAME0;
    if (frm_tab[pt_frame].fr_type == FR_TBL) {
        frm_tab[pt_frame].fr_refcnt--;
        if (frm_tab[pt_frame].fr_refcnt == 0) {

          page_directory[pd_index].pd_pres = 0;

          frm_tab[pt_frame].fr_status = FRM_UNMAPPED;
          frm_tab[pt_frame].fr_pid = -1;
          frm_tab[pt_frame].fr_vpno = -1;
          frm_tab[pt_frame].fr_refcnt = 0;
          frm_tab[pt_frame].fr_type = -1;
          frm_tab[pt_frame].fr_dirty = 0;
          frm_tab[pt_frame].ref_bit = 0;
        }
    }
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* frame_id)
{
  int i;
  for (i = 0; i < NFRAMES; i++) {
    if (frm_tab[i].fr_status == FRM_UNMAPPED) {
        *frame_id = i;
        return OK;
    }
  }

  // No free frame available, start to evict frame
  int victim_frame_index = get_victim_frame();
  if (victim_frame_index == SYSERR || victim_frame_index < 0 || victim_frame_index >= NFRAMES) {
    return SYSERR;
  }

  int pid = frm_tab[victim_frame_index].fr_pid;
  int vpage = frm_tab[victim_frame_index].fr_vpno;

  pd_t *page_directory = (pd_t *)proctab[pid].pdbr;
  int pd_index = PAGE_DIRECTORY_INDEX(vpage);
  int pt_index = PAGE_TABLE_INDEX(vpage);
  
  pd_t *pd_entry = &page_directory[pd_index];
  if (pd_entry->pd_pres) {
      pt_t *pt_base = (pt_t *)(pd_entry->pd_base * NBPG);
      pt_t *pt_entry = &pt_base[pt_index];

      if (pt_entry->pt_dirty) {
          frm_tab[victim_frame_index].fr_dirty = 1;
      }
  }  
  
  if (frm_tab[victim_frame_index].fr_dirty == 1) {
    int store, page_offset;
    if (bsm_lookup(pid, vpage * NBPG, &store, &page_offset) == SYSERR) {
        //kprintf("[get_frm] bsm_lookup() failed: pid=%d, vpage=%d, store=%d , page_offset=%d \n", pid, vpage, store, page_offset);
        //kprintf("Killing pid %d in pfint()\n", pid);
        kill(pid);
        //free_frm(victim_frame_index);
        return SYSERR;
    }
    
    // (FRAME0 + victim_frame_index) is a physical page number, therefore needs to be converted into address
    char *src = (char *)((FRAME0 + victim_frame_index) * NBPG);
    write_bs(src, store, page_offset);
  }

  free_frm(victim_frame_index);

  //enqueue_frame(victim_frame_index);
  *frame_id = victim_frame_index;

  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int frame_id)
{
  if (frame_id < 0 || frame_id >= NFRAMES) {
    //kprintf("[free_frm] Invalid frame ID: %d\n", frame_id);
    return SYSERR;
  }

  int pid = frm_tab[frame_id].fr_pid;
  int vpage = frm_tab[frame_id].fr_vpno;

  if (frm_tab[frame_id].fr_type == FR_PAGE && pid != -1 && vpage != -1) {
    remove_frame_from_queue(frame_id);
    invalidate_page(pid, vpage);
  }

  frm_tab[frame_id].fr_status = FRM_UNMAPPED;
  frm_tab[frame_id].fr_pid = -1;
  frm_tab[frame_id].fr_vpno = -1;
  frm_tab[frame_id].fr_refcnt = 0;
  frm_tab[frame_id].fr_type = -1;
  frm_tab[frame_id].fr_dirty = 0;
  frm_tab[frame_id].ref_bit = 0;
  return OK;
}


int frm_lookup(int pid, int vpno) {
    pd_t *pd = (pd_t *)proctab[pid].pdbr;
    int pd_index = PAGE_DIRECTORY_INDEX(vpno);
    int pt_index = PAGE_TABLE_INDEX(vpno);

    // Page directory entry not present
    if (pd[pd_index].pd_pres == 0)
        return SYSERR;

    pt_t *pt = (pt_t *)(pd[pd_index].pd_base * NBPG);

    // Page table entry not present
    if (pt[pt_index].pt_pres == 0)
        return SYSERR;

    int frm = pt[pt_index].pt_base - FRAME0;
    if (frm < 0 || frm >= NFRAMES)  // Basic sanity check
        return SYSERR;

    return frm;
}
