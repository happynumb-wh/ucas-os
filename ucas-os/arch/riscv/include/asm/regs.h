#ifndef INCLUDE_REGS_H_
#define INCLUDE_REGS_H_

/* This is for struct TrapFrame in scheduler.h
 * Stack layout for all exceptions:
 *
 * ptrace needs to have all regs on the stack. If the order here is changed,
 * it needs to be updated in include/asm-mips/ptrace.h
 *
 * The first PTRSIZE*5 bytes are argument save space for C subroutines.
 */

#define OFFSET_REG_ZERO         0

/* return address */
#define OFFSET_REG_RA           8

/* pointers */
#define OFFSET_REG_SP           16 // stack
#define OFFSET_REG_GP           24 // global
#define OFFSET_REG_TP           32 // thread

/* temporary */
#define OFFSET_REG_T0           40
#define OFFSET_REG_T1           48
#define OFFSET_REG_T2           56

/* saved register */
#define OFFSET_REG_S0           64
#define OFFSET_REG_S1           72

/* args */
#define OFFSET_REG_A0           80
#define OFFSET_REG_A1           88
#define OFFSET_REG_A2           96
#define OFFSET_REG_A3           104
#define OFFSET_REG_A4           112
#define OFFSET_REG_A5           120
#define OFFSET_REG_A6           128
#define OFFSET_REG_A7           136

/* saved register */
#define OFFSET_REG_S2           144
#define OFFSET_REG_S3           152
#define OFFSET_REG_S4           160
#define OFFSET_REG_S5           168
#define OFFSET_REG_S6           176
#define OFFSET_REG_S7           184
#define OFFSET_REG_S8           192
#define OFFSET_REG_S9           200
#define OFFSET_REG_S10          208
#define OFFSET_REG_S11          216

/* temporary register */
#define OFFSET_REG_T3           224
#define OFFSET_REG_T4           232
#define OFFSET_REG_T5           240
#define OFFSET_REG_T6           248

/* privileged register */
#define OFFSET_REG_SSTATUS      256
#define OFFSET_REG_SEPC         264
#define OFFSET_REG_SBADADDR     272
#define OFFSET_REG_SCAUSE       280
#define OFFSET_REG_SSCRATCH     288
/* Size of stack frame, word/double word alignment */
#define OFFSET_SIZE             296

/* N-extension */
#define OFFSET_REG_USTATUS      0
#define OFFSET_REG_UEPC         8
#define OFFSET_REG_UBADADDR     16
#define OFFSET_REG_UCAUSE       24
#define OFFSET_REG_UTVEC        32
#define OFFSET_REG_UIE          40
#define OFFSET_REG_UIP          48
#define OFFSET_REG_USCRATCH     56

#define OFFSET_N_EXTENSION      64

/* dasics supervisor registers */
#define OFFSET_DASICSUMAINCFG       0
#define OFFSET_DASICSUMAINBOUNDLO   8
#define OFFSET_DASICSUMAINBOUNDHI   16

/* dasics user registers */
#define OFFSET_DASICSLIBCFG0        24

#define OFFSET_DASICSLIBBOUND0LO            32
#define OFFSET_DASICSLIBBOUND0HI            40
#define OFFSET_DASICSLIBBOUND1LO            48
#define OFFSET_DASICSLIBBOUND1HI            56
#define OFFSET_DASICSLIBBOUND2LO            64
#define OFFSET_DASICSLIBBOUND2HI            72
#define OFFSET_DASICSLIBBOUND3LO            80
#define OFFSET_DASICSLIBBOUND3HI            88
#define OFFSET_DASICSLIBBOUND4LO            96
#define OFFSET_DASICSLIBBOUND4HI            104
#define OFFSET_DASICSLIBBOUND5LO            112
#define OFFSET_DASICSLIBBOUND5HI            120
#define OFFSET_DASICSLIBBOUND6LO            128
#define OFFSET_DASICSLIBBOUND6HI            136
#define OFFSET_DASICSLIBBOUND7LO            144
#define OFFSET_DASICSLIBBOUND7HI            152
#define OFFSET_DASICSLIBBOUND8LO            160
#define OFFSET_DASICSLIBBOUND8HI            168
#define OFFSET_DASICSLIBBOUND9LO            176
#define OFFSET_DASICSLIBBOUND9HI            184
#define OFFSET_DASICSLIBBOUND10LO           192
#define OFFSET_DASICSLIBBOUND10HI           200
#define OFFSET_DASICSLIBBOUND11LO           208
#define OFFSET_DASICSLIBBOUND11HI           216
#define OFFSET_DASICSLIBBOUND12LO           224
#define OFFSET_DASICSLIBBOUND12HI           232
#define OFFSET_DASICSLIBBOUND13LO           240
#define OFFSET_DASICSLIBBOUND13HI           248
#define OFFSET_DASICSLIBBOUND14LO           256
#define OFFSET_DASICSLIBBOUND14HI           264
#define OFFSET_DASICSLIBBOUND15LO           272
#define OFFSET_DASICSLIBBOUND15HI           280

#define OFFSET_DASICSMAINCALLENTRY          288
#define OFFSET_DASICSRETURNPC               296
#define OFFSET_DASICSFREEZONERETURNPC       304

#define OFFSET_DASICS_DJBOUND0LO            312
#define OFFSET_DASICS_DJBOUND0HI            320
#define OFFSET_DASICS_DJBOUND1LO            328
#define OFFSET_DASICS_DJBOUND1HI            336
#define OFFSET_DASICS_DJBOUND2LO            344
#define OFFSET_DASICS_DJBOUND2HI            352
#define OFFSET_DASICS_DJBOUND3LO            360
#define OFFSET_DASICS_DJBOUND3HI            368

#define OFFSET_DASICS_DJCFG                 376

#define OFFSET_DASICS                       384

#define OFFSET_CORE_ID              0


#define PCB_KERNEL_SP          0
#define PCB_USER_SP            8

/* offset in switch_to */
#define SWITCH_TO_RA     0
#define SWITCH_TO_SP     8
#define SWITCH_TO_S0     16
#define SWITCH_TO_S1     24
#define SWITCH_TO_S2     32
#define SWITCH_TO_S3     40
#define SWITCH_TO_S4     48
#define SWITCH_TO_S5     56
#define SWITCH_TO_S6     64
#define SWITCH_TO_S7     72
#define SWITCH_TO_S8     80
#define SWITCH_TO_S9     88
#define SWITCH_TO_S10    96
#define SWITCH_TO_S11    104
 
#define SWITCH_TO_SIZE   112

#endif
