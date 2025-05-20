#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
  // kprintf("To be implemented!\n");
  STATWORD ps;
  disable(ps);

  if(bs_id<0 || bs_id>=16)
  {
    restore(ps);
    return SYSERR;
  }

  if(bsm_tab[bs_id].bs_status==BSM_UNMAPPED)
  {
    restore(ps);
    return OK;
  }

  int i;
  for(i=0;i<NPROC;i++)
  {
    /* if bs_id matches in proctab */
    if(proctab[i].store==bs_id)
    {
      proctab[i].store=-1;
      proctab[i].vhpno=-1;
      proctab[i].vhpnpages=0;
      bsm_tab[i].bs_cnt_proc--;
      /* if current process is last process or it's private heap */
      if(bsm_tab[i].bs_require_heap==1 || bsm_tab[i].bs_cnt_proc==0)
      {
          free_bsm(i);
      }
    }
  }
  restore(ps);
  return OK;
}

