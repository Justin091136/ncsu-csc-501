#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL vfreemem(block, size)
struct mblock*block;
unsigned size;
{
STATWORD ps;
    struct mblock *prev, *curr;
    struct pentry *pptr;
	pptr = &proctab[currpid];

    unsigned long block_address, block_end, prev_end;

    disable(ps);

	/* check if size is 0, raise error */
	if (size == 0) {
        kprintf("\n[ERROR] Invalid size of 0 provided to vfreemem\n");
        restore(ps);
        return SYSERR;
    }

    block_address = (unsigned long)block;
    unsigned long min_addr = 4096 * NBPG;
    unsigned long max_addr = (proctab[currpid].vhpnpages * NBPG) + min_addr;
    /* check invalid address */
	if (block_address < min_addr || block_address > max_addr) {
        kprintf("\n[ERROR] vfreemem: Invalid block address 0x%08x\n", block_address);
        restore(ps);
        return SYSERR;
    }

	/* round the size */
    size = (unsigned int)roundmb(size);
    block_end = block_address + size;

    prev = pptr->vmemlist;
    curr = prev->mnext;
	/* iterate through list until the block */
    while (curr != NULL && curr < block) {
        prev = curr;
        curr = curr->mnext;
    }

    prev_end = (unsigned long)prev + prev->mlen;

	/* Check overlaps with adjacent free blocks (prev and curr)*/
    if ((prev_end > block_address && prev != pptr->vmemlist) ||
        (curr != NULL && block_end > (unsigned long)curr)) {
        kprintf("\n[ERROR] vfreemem: Block overlap detected\n");
        restore(ps);
        return SYSERR;
    }

	/* Merge free block with previous block if adjacent */
    if (prev != pptr->vmemlist && prev_end == block_address) {
        prev->mlen += size;
    } else {
        block->mlen = size;
        block->mnext = curr;
        prev->mnext = block;
        prev = block;
    }

	/* Merge with next block, if free block is adjacent */
    if (curr != NULL && ((unsigned long)prev + prev->mlen) == (unsigned long)curr) {
        prev->mlen += curr->mlen;
        prev->mnext = curr->mnext;
    }
    restore(ps);
    return OK;
}