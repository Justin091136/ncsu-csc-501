/* pfintr.S - pfintr */

    .text
pferrcode: .long 0
    .globl  pfintr, pferrcode

pfintr:
    popl pferrcode              # Error code is automatically pushed by CPU on page fault
    pushfl                      # Save EFLAGS (for restoring later)
    cli                         # Disable interrupts to prevent nested faults
    pushal                      # Save all general-purpose registers
    call pfint                  # Jump to C function to handle page fault
    popal
    popfl
    iret
