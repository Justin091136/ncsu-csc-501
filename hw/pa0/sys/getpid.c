/* getpid.c - getpid */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include "lab0.h"
/*------------------------------------------------------------------------
 * getpid  --  get the process id of currently executing process
 *------------------------------------------------------------------------
 */
SYSCALL getpid()
{	
	syscall_started(currpid, SYSCALL_GETPID);
	syscall_ended(currpid, SYSCALL_GETPID);
	return(currpid);
}
