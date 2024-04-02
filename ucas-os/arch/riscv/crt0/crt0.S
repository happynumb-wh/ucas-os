#include <asm.h>

.section ".entry_function","ax"
ENTRY(_start)
    /* call main */
    call main

    /* (P3) call sys_exit */
    call sys_exit

// while(1) loop, unreachable here
loop:
    wfi
    j loop

END(_start)
