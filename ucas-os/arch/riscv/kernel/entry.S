#include <asm.h>
#include <csr.h>
#include <asm/regs.h>

.macro SAVE_CONTEXT
  .local _restore_kernel_tpsp
  .local _save_context
  /*
   * If coming from userspace, preserve the user thread pointer and load
   * the kernel thread pointer.  If we came from the kernel, sscratch
   * will contain 0, and we should continue on the current TP.
   */
  csrrw tp, CSR_SSCRATCH, tp
  bnez tp, _save_context

_restore_kernel_tpsp:
  csrr tp, CSR_SSCRATCH
  sd sp, PCB_KERNEL_SP(tp)
_save_context:
  sd sp, PCB_USER_SP(tp)
  ld sp, PCB_KERNEL_SP(tp)
  addi sp, sp, -(OFFSET_SIZE)


  /* save all general purpose registers here! */
  // return address
  sd ra, OFFSET_REG_RA(sp)

  // pointers, sp and tp will be handled later
  sd gp, OFFSET_REG_GP(sp)

  // temporary
  sd t0, OFFSET_REG_T0(sp)
  sd t1, OFFSET_REG_T1(sp)
  sd t2, OFFSET_REG_T2(sp)

  // saved register
  sd s0, OFFSET_REG_S0(sp)
  sd s1, OFFSET_REG_S1(sp)

  // args
  sd a0, OFFSET_REG_A0(sp)
  sd a1, OFFSET_REG_A1(sp)
  sd a2, OFFSET_REG_A2(sp)
  sd a3, OFFSET_REG_A3(sp)
  sd a4, OFFSET_REG_A4(sp)
  sd a5, OFFSET_REG_A5(sp)
  sd a6, OFFSET_REG_A6(sp)
  sd a7, OFFSET_REG_A7(sp)

  // saved register
  sd s2, OFFSET_REG_S2(sp)
  sd s3, OFFSET_REG_S3(sp)
  sd s4, OFFSET_REG_S4(sp)
  sd s5, OFFSET_REG_S5(sp)
  sd s6, OFFSET_REG_S6(sp)
  sd s7, OFFSET_REG_S7(sp)
  sd s8, OFFSET_REG_S8(sp)
  sd s9, OFFSET_REG_S9(sp)
  sd s10, OFFSET_REG_S10(sp)
  sd s11, OFFSET_REG_S11(sp)

  // temporary register
  sd t3, OFFSET_REG_T3(sp)
  sd t4, OFFSET_REG_T4(sp)
  sd t5, OFFSET_REG_T5(sp)
  sd t6, OFFSET_REG_T6(sp)

  /*
   * Disable user-mode memory access as it should only be set in the
   * actual user copy routines.
   *
   * Disable the FPU to detect illegal usage of floating point in kernel
   * space.
   */
  li t0, SR_FS

  /* save sstatus, sepc, stval and scause on kernel stack */
  ld s0, PCB_USER_SP(tp)
  csrrc s1, CSR_SSTATUS, t0
  csrr s2, CSR_SEPC
  csrr s3, CSR_STVAL
  csrr s4, CSR_SCAUSE
  csrr s5, CSR_SSCRATCH  // The old value of tp
  sd s0,   OFFSET_REG_SP(sp)
  sd s1,   OFFSET_REG_SSTATUS(sp)
  sd s2,   OFFSET_REG_SEPC(sp)
  sd s3,   OFFSET_REG_SBADADDR(sp)
  sd s4,   OFFSET_REG_SCAUSE(sp)
  sd s5,   OFFSET_REG_TP(sp)
.endm

.macro RESTORE_CONTEXT
  /* Restore all registers and sepc,sstatus */
  // priviledge register
  ld t0, OFFSET_REG_SSTATUS(sp)
  ld t1, OFFSET_REG_SEPC(sp)
  ld t2, OFFSET_REG_SSCRATCH(sp)
  csrw CSR_SSTATUS, t0
  csrw CSR_SEPC, t1
  csrw CSR_SSCRATCH, t2

  // return address
  ld ra, OFFSET_REG_RA(sp)

  // pointers, sp will be handled later
  ld gp, OFFSET_REG_GP(sp)
  ld tp, OFFSET_REG_TP(sp)

  // temporary
  ld t0, OFFSET_REG_T0(sp)
  ld t1, OFFSET_REG_T1(sp)
  ld t2, OFFSET_REG_T2(sp)

  // saved register
  ld s0, OFFSET_REG_S0(sp)
  ld s1, OFFSET_REG_S1(sp)

  // args
  ld a0, OFFSET_REG_A0(sp)
  ld a1, OFFSET_REG_A1(sp)
  ld a2, OFFSET_REG_A2(sp)
  ld a3, OFFSET_REG_A3(sp)
  ld a4, OFFSET_REG_A4(sp)
  ld a5, OFFSET_REG_A5(sp)
  ld a6, OFFSET_REG_A6(sp)
  ld a7, OFFSET_REG_A7(sp)

  // saved register
  ld s2, OFFSET_REG_S2(sp)
  ld s3, OFFSET_REG_S3(sp)
  ld s4, OFFSET_REG_S4(sp)
  ld s5, OFFSET_REG_S5(sp)
  ld s6, OFFSET_REG_S6(sp)
  ld s7, OFFSET_REG_S7(sp)
  ld s8, OFFSET_REG_S8(sp)
  ld s9, OFFSET_REG_S9(sp)
  ld s10, OFFSET_REG_S10(sp)
  ld s11, OFFSET_REG_S11(sp)

  // temporary register
  ld t3, OFFSET_REG_T3(sp)
  ld t4, OFFSET_REG_T4(sp)
  ld t5, OFFSET_REG_T5(sp)
  ld t6, OFFSET_REG_T6(sp)

  // sp register
  ld sp,  OFFSET_REG_SP(sp)
.endm

.macro SAVE_N_EXTENSION
  addi sp, sp, -(OFFSET_N_EXTENSION)
#ifdef DASICS

#ifndef NANHU_V3
  csrr t0, CSR_USTATUS
  sd t0, OFFSET_REG_USTATUS(sp)

  csrr t0, CSR_UIE
  sd t0, OFFSET_REG_UIE(sp)

  csrr t0, CSR_UTVEC
  sd t0, OFFSET_REG_UTVEC(sp)

  csrr t0, CSR_USCRATCH
  sd t0, OFFSET_REG_USCRATCH(sp)

  csrr t0, CSR_UEPC
  sd t0, OFFSET_REG_UEPC(sp)

  csrr t0, CSR_UCAUSE
  sd t0, OFFSET_REG_UCAUSE(sp)

  csrr t0, CSR_UTVAL
  sd t0, OFFSET_REG_UBADADDR(sp)

  csrr t0, CSR_UIP
  sd t0, OFFSET_REG_UIP(sp)   
#endif

#endif
.endm


.macro RESTORE_N_EXTENSION

#ifdef DASICS

#ifndef NANHU_V3
  ld t0, OFFSET_REG_USTATUS(sp)
  csrw CSR_USTATUS, t0 

  ld t0, OFFSET_REG_UIE(sp)
  csrw CSR_UIE, t0 
  
  ld t0, OFFSET_REG_UTVEC(sp)
  csrw CSR_UTVEC, t0 

  ld t0, OFFSET_REG_USCRATCH(sp)
  csrw CSR_USCRATCH, t0
  
  ld t0, OFFSET_REG_UEPC(sp)
  csrw CSR_UEPC, t0 

  ld t0, OFFSET_REG_UCAUSE(sp)
  csrw CSR_UCAUSE, t0 

  ld t0, OFFSET_REG_UBADADDR(sp)
  csrw CSR_UTVAL, t0 

  ld t0, OFFSET_REG_UIP(sp)   
  csrw CSR_UIP, t0
#endif

#endif

  addi sp, sp, OFFSET_N_EXTENSION
.endm


.macro SAVE_DASICS
  addi sp, sp, -(OFFSET_DASICS)

#ifdef DASICS
  csrr t0, CSR_DUMCFG
  sd t0, OFFSET_DASICSUMAINCFG(sp)

  csrr t0, CSR_DUMBOUNDLO
  sd t0, OFFSET_DASICSUMAINBOUNDLO(sp)

  csrr t0, CSR_DUMBOUNDHI
  sd t0, OFFSET_DASICSUMAINBOUNDHI(sp)

  csrr t0, CSR_DLCFG0
  sd t0, OFFSET_DASICSLIBCFG0(sp)

  csrr t0, CSR_DLBOUND0LO
  sd t0, OFFSET_DASICSLIBBOUND0LO(sp)

  csrr t0, CSR_DLBOUND0HI
  sd t0, OFFSET_DASICSLIBBOUND0HI(sp)

  csrr t0, CSR_DLBOUND1LO
  sd t0, OFFSET_DASICSLIBBOUND1LO(sp)

  csrr t0, CSR_DLBOUND1HI
  sd t0, OFFSET_DASICSLIBBOUND1HI(sp)

  csrr t0, CSR_DLBOUND2LO
  sd t0, OFFSET_DASICSLIBBOUND2LO(sp)

  csrr t0, CSR_DLBOUND2HI
  sd t0, OFFSET_DASICSLIBBOUND2HI(sp)

  csrr t0, CSR_DLBOUND3LO
  sd t0, OFFSET_DASICSLIBBOUND3LO(sp)

  csrr t0, CSR_DLBOUND3HI
  sd t0, OFFSET_DASICSLIBBOUND3HI(sp)

#ifndef NANHU_V3
  csrr t0, CSR_DLBOUND4LO
  sd t0, OFFSET_DASICSLIBBOUND4LO(sp)

  csrr t0, CSR_DLBOUND4HI
  sd t0, OFFSET_DASICSLIBBOUND4HI(sp)

  csrr t0, CSR_DLBOUND5LO
  sd t0, OFFSET_DASICSLIBBOUND5LO(sp)

  csrr t0, CSR_DLBOUND5HI
  sd t0, OFFSET_DASICSLIBBOUND5HI(sp)

  csrr t0, CSR_DLBOUND6LO
  sd t0, OFFSET_DASICSLIBBOUND6LO(sp)
  
  csrr t0, CSR_DLBOUND6HI
  sd t0, OFFSET_DASICSLIBBOUND6HI(sp)

  csrr t0, CSR_DLBOUND7LO
  sd t0, OFFSET_DASICSLIBBOUND7LO(sp)

  csrr t0, CSR_DLBOUND7HI
  sd t0, OFFSET_DASICSLIBBOUND7HI(sp)

  csrr t0, CSR_DLBOUND8LO
  sd t0, OFFSET_DASICSLIBBOUND8LO(sp)

  csrr t0, CSR_DLBOUND8HI
  sd t0, OFFSET_DASICSLIBBOUND8HI(sp)

  csrr t0, CSR_DLBOUND9LO
  sd t0, OFFSET_DASICSLIBBOUND9LO(sp)

  csrr t0, CSR_DLBOUND9HI
  sd t0, OFFSET_DASICSLIBBOUND9HI(sp)

  csrr t0, CSR_DLBOUND10LO
  sd t0, OFFSET_DASICSLIBBOUND10LO(sp)

  csrr t0, CSR_DLBOUND10HI
  sd t0, OFFSET_DASICSLIBBOUND10HI(sp)

  csrr t0, CSR_DLBOUND11LO
  sd t0, OFFSET_DASICSLIBBOUND11LO(sp)

  csrr t0, CSR_DLBOUND11HI
  sd t0, OFFSET_DASICSLIBBOUND11HI(sp)

  csrr t0, CSR_DLBOUND12LO
  sd t0, OFFSET_DASICSLIBBOUND12LO(sp)

  csrr t0, CSR_DLBOUND12HI
  sd t0, OFFSET_DASICSLIBBOUND12HI(sp)

  csrr t0, CSR_DLBOUND13LO
  sd t0, OFFSET_DASICSLIBBOUND13LO(sp)

  csrr t0, CSR_DLBOUND13HI
  sd t0, OFFSET_DASICSLIBBOUND13HI(sp)

  csrr t0, CSR_DLBOUND14LO
  sd t0, OFFSET_DASICSLIBBOUND14LO(sp)

  csrr t0, CSR_DLBOUND14HI
  sd t0, OFFSET_DASICSLIBBOUND14HI(sp)

  csrr t0, CSR_DLBOUND15LO
  sd t0, OFFSET_DASICSLIBBOUND15LO(sp)

  csrr t0, CSR_DLBOUND15HI
  sd t0, OFFSET_DASICSLIBBOUND15HI(sp)

  csrr t0, CSR_DFZRETURN
  sd t0, OFFSET_DASICSFREEZONERETURNPC(sp)
#endif

  csrr t0, CSR_DMAINCALL
  sd t0, OFFSET_DASICSMAINCALLENTRY(sp)

  csrr t0, CSR_DRETURNPC
  sd t0, OFFSET_DASICSRETURNPC(sp)

  csrr t0, CSR_DJBOUND0LO
  sd t0, OFFSET_DASICS_DJBOUND0LO(sp)

  csrr t0, CSR_DJBOUND0HI
  sd t0, OFFSET_DASICS_DJBOUND0HI(sp)

#ifndef NANHU_V3
  csrr t0, CSR_DJBOUND1LO
  sd t0, OFFSET_DASICS_DJBOUND1LO(sp)

  csrr t0, CSR_DJBOUND1HI
  sd t0, OFFSET_DASICS_DJBOUND1HI(sp)

  csrr t0, CSR_DJBOUND2LO
  sd t0, OFFSET_DASICS_DJBOUND2LO(sp)

  csrr t0, CSR_DJBOUND2HI
  sd t0, OFFSET_DASICS_DJBOUND2HI(sp)

  csrr t0, CSR_DJBOUND3LO
  sd t0, OFFSET_DASICS_DJBOUND3LO(sp)

  csrr t0, CSR_DJBOUND3HI
  sd t0, OFFSET_DASICS_DJBOUND3HI(sp)
#endif

  csrr t0, CSR_DJCFG
  sd t0, OFFSET_DASICS_DJCFG(sp)

#endif
.endm


.macro RESTORE_DASICS
#ifdef DASICS
  ld t0, OFFSET_DASICSUMAINCFG(sp)
  csrw CSR_DUMCFG, t0

  ld t0, OFFSET_DASICSUMAINBOUNDLO(sp)
  csrw CSR_DUMBOUNDLO, t0

  ld t0, OFFSET_DASICSUMAINBOUNDHI(sp)
  csrw CSR_DUMBOUNDHI, t0 

  ld t0, OFFSET_DASICSLIBCFG0(sp)
  csrw CSR_DLCFG0, t0

  ld t0, OFFSET_DASICSLIBBOUND0LO(sp)
  csrw CSR_DLBOUND0LO, t0

  ld t0, OFFSET_DASICSLIBBOUND0HI(sp)
  csrw CSR_DLBOUND0HI, t0

  ld t0, OFFSET_DASICSLIBBOUND1LO(sp)
  csrw CSR_DLBOUND1LO, t0

  ld t0, OFFSET_DASICSLIBBOUND1HI(sp)
  csrw CSR_DLBOUND1HI, t0

  ld t0, OFFSET_DASICSLIBBOUND2LO(sp)
  csrw CSR_DLBOUND2LO, t0

  ld t0, OFFSET_DASICSLIBBOUND2HI(sp)
  csrw CSR_DLBOUND2HI, t0

  ld t0, OFFSET_DASICSLIBBOUND3LO(sp)
  csrw CSR_DLBOUND3LO, t0

  ld t0, OFFSET_DASICSLIBBOUND3HI(sp)
  csrw CSR_DLBOUND3HI, t0

#ifndef NANHU_V3
  ld t0, OFFSET_DASICSLIBBOUND4LO(sp)
  csrw CSR_DLBOUND4LO, t0

  ld t0, OFFSET_DASICSLIBBOUND4HI(sp)
  csrw CSR_DLBOUND4HI, t0

  ld t0, OFFSET_DASICSLIBBOUND5LO(sp)
  csrw CSR_DLBOUND5LO, t0

  ld t0, OFFSET_DASICSLIBBOUND5HI(sp)
  csrw CSR_DLBOUND5HI, t0

  ld t0, OFFSET_DASICSLIBBOUND6LO(sp)
  csrw CSR_DLBOUND6LO, t0
  
  ld t0, OFFSET_DASICSLIBBOUND6HI(sp)
  csrw CSR_DLBOUND6HI, t0

  ld t0, OFFSET_DASICSLIBBOUND7LO(sp)
  csrw CSR_DLBOUND7LO, t0

  ld t0, OFFSET_DASICSLIBBOUND7HI(sp)
  csrw CSR_DLBOUND7HI, t0

  ld t0, OFFSET_DASICSLIBBOUND8LO(sp)
  csrw CSR_DLBOUND8LO, t0

  ld t0, OFFSET_DASICSLIBBOUND8HI(sp)
  csrw CSR_DLBOUND8HI, t0

  ld t0, OFFSET_DASICSLIBBOUND9LO(sp)
  csrw CSR_DLBOUND9LO, t0

  ld t0, OFFSET_DASICSLIBBOUND9HI(sp)
  csrw CSR_DLBOUND9HI, t0

  ld t0, OFFSET_DASICSLIBBOUND10LO(sp)
  csrw CSR_DLBOUND10LO, t0

  ld t0, OFFSET_DASICSLIBBOUND10HI(sp)
  csrw CSR_DLBOUND10HI, t0

  ld t0, OFFSET_DASICSLIBBOUND11LO(sp)
  csrw CSR_DLBOUND11LO, t0

  ld t0, OFFSET_DASICSLIBBOUND11HI(sp)
  csrw CSR_DLBOUND11HI, t0

  ld t0, OFFSET_DASICSLIBBOUND12LO(sp)
  csrw CSR_DLBOUND12LO, t0

  ld t0, OFFSET_DASICSLIBBOUND12HI(sp)
  csrw CSR_DLBOUND12HI, t0

  ld t0, OFFSET_DASICSLIBBOUND13LO(sp)
  csrw CSR_DLBOUND13LO, t0

  ld t0, OFFSET_DASICSLIBBOUND13HI(sp)
  csrw CSR_DLBOUND13HI, t0

  ld t0, OFFSET_DASICSLIBBOUND14LO(sp)
  csrw CSR_DLBOUND14LO, t0

  ld t0, OFFSET_DASICSLIBBOUND14HI(sp)
  csrw CSR_DLBOUND14HI, t0

  ld t0, OFFSET_DASICSLIBBOUND15LO(sp)
  csrw CSR_DLBOUND15LO, t0

  ld t0, OFFSET_DASICSLIBBOUND15HI(sp)
  csrw CSR_DLBOUND15HI, t0
  
  ld t0, OFFSET_DASICSFREEZONERETURNPC(sp)   
  csrw CSR_DFZRETURN, t0
#endif

  ld t0, OFFSET_DASICSMAINCALLENTRY(sp)
  csrw CSR_DMAINCALL, t0

  ld t0, OFFSET_DASICSRETURNPC(sp)
  csrw CSR_DRETURNPC, t0 

  ld t0, OFFSET_DASICS_DJBOUND0LO(sp)
  csrw CSR_DJBOUND0LO, t0

  ld t0, OFFSET_DASICS_DJBOUND0HI(sp)
  csrw CSR_DJBOUND0HI, t0

#ifndef NANHU_V3
  ld t0, OFFSET_DASICS_DJBOUND1LO(sp)
  csrw CSR_DJBOUND1LO, t0

  ld t0, OFFSET_DASICS_DJBOUND1HI(sp)
  csrw CSR_DJBOUND1HI, t0

  ld t0, OFFSET_DASICS_DJBOUND2LO(sp)
  csrw CSR_DJBOUND2LO, t0

  ld t0, OFFSET_DASICS_DJBOUND2HI(sp)
  csrw CSR_DJBOUND2HI, t0

  ld t0, OFFSET_DASICS_DJBOUND3LO(sp)
  csrw CSR_DJBOUND3LO, t0

  ld t0, OFFSET_DASICS_DJBOUND3HI(sp)
  csrw CSR_DJBOUND3HI, t0

#endif

  ld t0, OFFSET_DASICS_DJCFG(sp)
  csrw CSR_DJCFG, t0

#endif

  addi sp, sp, OFFSET_DASICS
.endm


ENTRY(enable_preempt)
  not t0, x0
  csrs CSR_SIE, t0
  jr ra
ENDPROC(enable_preempt)

ENTRY(disable_preempt)
  csrw CSR_SIE, zero
  jr ra
ENDPROC(disable_preempt)

ENTRY(enable_interrupt)
  li t0, SR_SIE
  csrs CSR_SSTATUS, t0
  jr ra
ENDPROC(enable_interrupt)

ENTRY(disable_interrupt)
  li t0, SR_SIE
  csrc CSR_SSTATUS, t0
  jr ra
ENDPROC(disable_interrupt)

// the address of previous pcb in a0
// the address of next pcb in a1
ENTRY(switch_to)
  // save all callee save registers on kernel stack
  addi sp, sp, -(SWITCH_TO_SIZE)
  sd ra, SWITCH_TO_RA(sp)
  sd s0, SWITCH_TO_S0(sp)
  sd s1, SWITCH_TO_S1(sp)
  sd s2, SWITCH_TO_S2(sp)
  sd s3, SWITCH_TO_S3(sp)
  sd s4, SWITCH_TO_S4(sp)
  sd s5, SWITCH_TO_S5(sp)
  sd s6, SWITCH_TO_S6(sp)
  sd s7, SWITCH_TO_S7(sp)
  sd s8, SWITCH_TO_S8(sp)
  sd s9, SWITCH_TO_S9(sp)
  sd s10, SWITCH_TO_S10(sp)
  sd s11, SWITCH_TO_S11(sp)

  sd sp, SWITCH_TO_SP(sp)
  sd sp, PCB_KERNEL_SP(a0)  // previous pcb

  // restore next
  mv tp, a1  // next pcb
  ld sp, PCB_KERNEL_SP(tp)

  ld ra, SWITCH_TO_RA(sp)
  ld s0, SWITCH_TO_S0(sp)
  ld s1, SWITCH_TO_S1(sp)
  ld s2, SWITCH_TO_S2(sp)
  ld s3, SWITCH_TO_S3(sp)
  ld s4, SWITCH_TO_S4(sp)
  ld s5, SWITCH_TO_S5(sp)
  ld s6, SWITCH_TO_S6(sp)
  ld s7, SWITCH_TO_S7(sp)
  ld s8, SWITCH_TO_S8(sp)
  ld s9, SWITCH_TO_S9(sp)
  ld s10, SWITCH_TO_S10(sp)
  ld s11, SWITCH_TO_S11(sp)
  ld sp,  SWITCH_TO_SP(sp)

  addi sp, sp, SWITCH_TO_SIZE
  jr ra
ENDPROC(switch_to)

ENTRY(ret_from_exception)
  /* check if we return to kernel or a normal thread */
  la t0, pid0_pcb_master
  beq tp, t0, restore
  la t0, pid0_pcb_slave
  beq tp, t0, restore

  /* return to a normal thread(not the pid0 thread) now */
  addi t0, sp, OFFSET_SIZE
  addi t0, t0, OFFSET_DASICS
  addi t0, t0, OFFSET_N_EXTENSION
  sd t0, PCB_KERNEL_SP(tp)

restore:
  /* call unlock kernel */
  // call unlock_kernel
  RESTORE_DASICS
  RESTORE_N_EXTENSION
  RESTORE_CONTEXT
  sret
ENDPROC(ret_from_exception)

ENTRY(exception_handler_entry)
  SAVE_CONTEXT
  SAVE_N_EXTENSION
  SAVE_DASICS

  /* call lock_kernel */
  // call lock_kernel

  csrw CSR_SSCRATCH, x0

  /* load ret_from_exception into $ra
   * so that we can return to ret_from_exception
   * when interrupt_help complete.
   */

  /* Load the global pointer again */  
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop  

  la ra, ret_from_exception

  /* call interrupt_helper
   * note: don't forget to pass parameters for it.
   */
  mv a0, sp
  csrr a1, CSR_STVAL
  csrr a2, CSR_SCAUSE

  j interrupt_helper

ENDPROC(exception_handler_entry)
