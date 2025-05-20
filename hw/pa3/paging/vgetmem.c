#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];

WORD *vgetmem(unsigned int nbytes) {
    STATWORD ps;
    struct mblock *curr, *prev, *remaining;
    struct pentry *pptr;
    pptr = &proctab[currpid];

    disable(ps);
    
	/* check if nbytes is 0 and raise error */
    if (nbytes == 0) {
        kprintf("\n[ERROR] Invalid value sent to vgetmem\n");
        restore(ps);
        return (WORD *)SYSERR;
    }

	/* check if vmemlist has no next pointer and raise error */
    if (pptr->vmemlist->mnext == NULL) {
        kprintf("\n[ERROR] vgetmem: Empty memory list\n");
        restore(ps);
        return (WORD *)SYSERR;
    }

	/* round the nbytes*/
    nbytes = (unsigned int)roundmb(nbytes);
    
    prev = pptr->vmemlist;
    curr = prev->mnext;
	/* iterate through the list */
    while (curr != NULL) {
		/* block found with exact same nbytes */
        if (curr->mlen == nbytes) {
            prev->mnext = curr->mnext;
            restore(ps);
            return (WORD *)curr;
        }
		/* current block has size greater than required size */
        if (curr->mlen > nbytes) {
            remaining = (struct mblock *)((unsigned)curr + nbytes);
			/* calculate remaining bytes in that block */
            remaining->mlen = curr->mlen - nbytes;
            remaining->mnext = curr->mnext;
            prev->mnext = remaining;
            restore(ps);
            return (WORD *)curr;
        }
		/* update the pointers */
        prev = curr;
        curr = curr->mnext;
    }

    kprintf("\n[ERROR] vgetmem: No suitable block found\n");
    restore(ps);
    return (WORD *)SYSERR;
}