#include <asm.h>
#include <csr.h>
#include <asm/regs.h>

ENTRY(set_signal_trampoline)
    .local copy_signal_switch_regs
    .local copy_signal_context_regs
    /* sd tp */
    csrrw tp, CSR_SSCRATCH, tp
    ld  a3, PCB_KERNEL_SP(tp)

    mv a4, a3 
    li a5, SWITCH_TO_SIZE

    /* copy switch_regs */
copy_signal_switch_regs:
    ld a6, (a1)
    sd a6, (a3)
    addi a3, a3, 8
    addi a1, a1, 8
    addi a5, a5, -8
    bnez a5, copy_signal_switch_regs

    li a5, OFFSET_SIZE

copy_signal_context_regs:
    ld a6, (a0)
    sd a6, (a3)
    addi a3, a3, 8
    addi a0, a0, 8
    addi a5, a5, -8
    bnez a5, copy_signal_context_regs    

    /* change the sp */
    ld  sp, PCB_KERNEL_SP(tp)

    call ret_from_exception

END(set_signal_trampoline)



ENTRY(exit_signal_trampoline)
    
    call ret_from_exception

END(exit_signal_trampoline)