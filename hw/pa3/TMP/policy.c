/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

extern int page_replace_policy;

int page_queue[NFRAMES]; // For FIFO
int queue_head = 0, queue_tail = 0;
int queue_size = 0;

// Remove from queue head
int dequeue_frame(void) {
  if (queue_size == 0) {
      //kprintf("Queue is empty!\n");
      return SYSERR;
  }
  int frame = page_queue[queue_head];

  //kprintf("[dequeue_frame] Dequeuing frame %d from queue for pid %d\n", frame, currpid);

  queue_head = (queue_head + 1) % NFRAMES;
  queue_size--;
  return frame;
}

int remove_frame_from_queue(int frame) {
    if (queue_size == 0 || frame < 0 || frame >= NFRAMES) {
        return SYSERR;
    }

    //kprintf("Removing frame %d from queue for pid %d\n", frame, currpid);
    int new_queue[NFRAMES];
    int new_tail = 0;
    int i = queue_head;
    int count = 0;
    int removed = 0;

    while (count < queue_size) {
        if (page_queue[i] != frame) {
            new_queue[new_tail++] = page_queue[i];
        } else {
            removed = 1;
        }
        i = (i + 1) % NFRAMES;
        count++;
    }

    if (!removed) {
        return SYSERR; // frame not found
    }

    // Copy back to page_queue
    for (i = 0; i < new_tail; i++) {
        page_queue[i] = new_queue[i];
    }

    queue_head = 0;
    queue_tail = new_tail % NFRAMES;
    queue_size = new_tail;

    return OK;
}


// To record who enter the frame first
int enqueue_frame(int frame) {
  if (queue_size == NFRAMES) {
      //kprintf("Queue full! Should not happen!\n");
      return SYSERR;
  }
  
  int i, idx;
  for (i = 0, idx = queue_head; i < queue_size; i++, idx = (idx + 1) % NFRAMES) {
      if (page_queue[idx] == frame) return OK; // Already in queue
  }

  page_queue[queue_tail] = frame;
  queue_tail = (queue_tail + 1) % NFRAMES;
  queue_size++;
  return OK;
}

/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  /* sanity check ! */
  if (policy != SC && policy != FIFO) {
    //kprintf("Incorrect policy type\n");
    return SYSERR;
  }
  page_replace_policy = policy;
  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}

/*
  FIFO related function
*/
int get_victim_fifo() {
  int attempts = queue_size;

  while (attempts > 0) {
      int frame = dequeue_frame();
      if (frame == SYSERR) {
        return SYSERR;
      }
      attempts--;
      
      if (frm_tab[frame].fr_type == FR_PAGE) {
          // Only return when the victim is not page table or page directory
          return frame;
      }

      if (frm_tab[frame].fr_type != FR_PAGE) {
        //kprintf("Found page table/page directory in queue, they should be there and now removing it\n");
      }
  }

  //kprintf("[FIFO] No evictable frame found!\n");
  return SYSERR;
}

static void sync_ref_bit(int frame)
{
    unsigned vpno = frm_tab[frame].fr_vpno;
    int pid = frm_tab[frame].fr_pid;

    pd_t *pd = (pd_t *)proctab[pid].pdbr;
    pd_t *pd_entry  = &pd[PAGE_DIRECTORY_INDEX(vpno)];
    if (pd_entry->pd_pres == 0) return;

    pt_t *pt_base   = (pt_t *)(pd_entry->pd_base * NBPG);
    pt_t *pt_entry  = &pt_base[PAGE_TABLE_INDEX(vpno)];

    if (pt_entry->pt_acc) {
        frm_tab[frame].ref_bit = 1;
        pt_entry->pt_acc = 0;
    }
}

int get_victim_sc() {
  if (queue_size == 0) {
      //kprintf("[SC] Error: victim queue is empty!\n");
      return SYSERR;
  }

  int original_size = queue_size;

  int i;
  for (i = 0; i < original_size; i++) {
      int frame = dequeue_frame();
      if (frame == SYSERR) {
        return SYSERR;
      }

      // Ignore page table or page directory
      if (frm_tab[frame].fr_type != FR_PAGE) {
          //kprintf("Found page table/page directory in queue, they should be there and now removing it\n");
          //enqueue_frame(frame);
          continue;
      }

      sync_ref_bit(frame);

      if (frm_tab[frame].ref_bit == 0) {
          return frame;
      } else {
          frm_tab[frame].ref_bit = 0;
          enqueue_frame(frame);
      }
  }

  // Scan again in case no victim found in last round
  for (i = 0; i < queue_size; i++) {
      int frame = dequeue_frame();
      if (frame == SYSERR) {
          return SYSERR;
      }

      if (frm_tab[frame].fr_type != FR_PAGE) {
          enqueue_frame(frame);
          continue;
      }

      // All ref_bits have been reset in the first scan, so no further check is needed
      return frame;
  }

  //kprintf("[SC] No evictable frame found!\n");
  return SYSERR;
}


int get_victim_frame() {
  if (page_replace_policy == FIFO)
      return get_victim_fifo();
  else
      return get_victim_sc();
}
