#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

    STATWORD ps;
    disable(ps);
    /* saftey check */
    if(bs_id<0 || bs_id>=16 || npages<=0 || npages>128 || bsm_tab[bs_id].bs_status==BSM_MAPPED && (bsm_tab[bs_id].bs_require_heap==1 || npages>(128-bsm_tab[bs_id].bs_npages)))
    {
      restore(ps);
      return SYSERR;
    }
    if(bsm_tab[bs_id].bs_status==BSM_UNMAPPED)
    {
      restore(ps);
      return npages;
    }
    restore(ps);
    return bsm_tab[bs_id].bs_npages;
}


