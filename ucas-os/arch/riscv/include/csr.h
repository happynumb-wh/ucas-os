/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015 Regents of the University of California
 */

#ifndef CSR_H
#define CSR_H
#include "encoding.h"

/* Status register flags */
#define SR_SIE    0x00000002 /* Supervisor Interrupt Enable */
#define SR_SPIE   0x00000020 /* Previous Supervisor IE */
#define SR_SPP    0x00000100 /* Previously Supervisor */
#define SR_SUM    0x00040000 /* Supervisor User Memory Access */

#define SR_FS           0x00006000 /* Floating-point Status */
#define SR_FS_OFF       0x00000000
#define SR_FS_INITIAL   0x00002000
#define SR_FS_CLEAN     0x00004000
#define SR_FS_DIRTY     0x00006000

#define SR_XS           0x00018000 /* Extension Status */
#define SR_XS_OFF       0x00000000
#define SR_XS_INITIAL   0x00008000
#define SR_XS_CLEAN     0x00010000
#define SR_XS_DIRTY     0x00018000

#define SR_SD           0x8000000000000000 /* FS/XS dirty */

/* SATP flags */
#define SATP_PPN        0x00000FFFFFFFFFFF
#define SATP_MODE_39    0x8000000000000000
#define SATP_MODE       SATP_MODE_39

/* SCAUSE */
#define SCAUSE_IRQ_FLAG   (1UL << 63)

#define IRQ_U_SOFT		0
#define IRQ_S_SOFT		1
#define IRQ_M_SOFT		3
#define IRQ_U_TIMER		4
#define IRQ_S_TIMER		5
#define IRQ_M_TIMER		7
#define IRQ_U_EXT		8
#define IRQ_S_EXT		9
#define IRQ_M_EXT		11

#define EXC_INST_MISALIGNED	0
#define EXC_INST_ACCESS		1
#define EXC_BREAKPOINT		3
#define EXC_LOAD_ACCESS		5
#define EXC_STORE_ACCESS	7
#define EXC_SYSCALL		8
#define EXC_INST_PAGE_FAULT	12
#define EXC_LOAD_PAGE_FAULT	13
#define EXC_STORE_PAGE_FAULT	15



// Nanhu v3 slow down the number of dasics
#ifndef NANHU_V3
/* U state csrs */
#define CSR_USTATUS         0x000
#define CSR_UIE             0x004
#define CSR_UTVEC           0x005
#define CSR_USCRATCH        0x040
#define CSR_UEPC            0x041
#define CSR_UCAUSE          0x042
#define CSR_UTVAL           0x043
#define CSR_UIP             0x044

#endif

/* DASICS csrs */
#define CSR_DUMCFG          0x9e0
#define CSR_DUMBOUNDLO      0x9e2
#define CSR_DUMBOUNDHI      0x9e3

/* DASICS Main cfg */
#define DASICS_MAINCFG_MASK 0xfUL
#define DASICS_UCFG_CLS     0x8UL
#define DASICS_SCFG_CLS     0x4UL
#define DASICS_UCFG_ENA     0x2UL
#define DASICS_SCFG_ENA     0x1UL

/* DASICS Lib csrs */
#define CSR_DLCFG0          0x880

#define CSR_DLBOUND0LO      0x890
#define CSR_DLBOUND0HI      0x891
#define CSR_DLBOUND1LO      0x892
#define CSR_DLBOUND1HI      0x893
#define CSR_DLBOUND2LO      0x894
#define CSR_DLBOUND2HI      0x895
#define CSR_DLBOUND3LO      0x896
#define CSR_DLBOUND3HI      0x897


#ifndef NANHU_V3
#define CSR_DLBOUND4LO      0x898
#define CSR_DLBOUND4HI      0x899
#define CSR_DLBOUND5LO      0x89a
#define CSR_DLBOUND5HI      0x89b
#define CSR_DLBOUND6LO      0x89c
#define CSR_DLBOUND6HI      0x89d
#define CSR_DLBOUND7LO      0x89e
#define CSR_DLBOUND7HI      0x89f
#define CSR_DLBOUND8LO      0x8a0
#define CSR_DLBOUND8HI      0x8a1
#define CSR_DLBOUND9LO      0x8a2
#define CSR_DLBOUND9HI      0x8a3
#define CSR_DLBOUND10LO     0x8a4
#define CSR_DLBOUND10HI     0x8a5
#define CSR_DLBOUND11LO     0x8a6
#define CSR_DLBOUND11HI     0x8a7
#define CSR_DLBOUND12LO     0x8a8
#define CSR_DLBOUND12HI     0x8a9
#define CSR_DLBOUND13LO     0x8aa
#define CSR_DLBOUND13HI     0x8ab
#define CSR_DLBOUND14LO     0x8ac
#define CSR_DLBOUND14HI     0x8ad
#define CSR_DLBOUND15LO     0x8ae
#define CSR_DLBOUND15HI     0x8af

#define CSR_DFZRETURN       0x8b2

#define CSR_DJBOUND1LO      0x8c2
#define CSR_DJBOUND1HI      0x8c3
#define CSR_DJBOUND2LO      0x8c4
#define CSR_DJBOUND2HI      0x8c5
#define CSR_DJBOUND3LO      0x8c6
#define CSR_DJBOUND3HI      0x8c7
#endif


#define CSR_DMAINCALL       0x8b0
#define CSR_DRETURNPC       0x8b1


#define CSR_DJBOUND0LO      0x8c0
#define CSR_DJBOUND0HI      0x8c1


#define CSR_DJCFG           0x8c8


/* DASICS Lib cfg */
#define DASICS_LIBCFG_WIDTH 4
#define DASICS_LIBCFG_MASK  0xfUL
#define DASICS_LIBCFG_V     0x8UL
#define DASICS_LIBCFG_R     0x2UL
#define DASICS_LIBCFG_W     0x1UL

#define DASICS_JUMPCFG_WIDTH 	1
#define DASICS_JUMPCFG_MASK 	0xffffUL
#define DASICS_JUMPCFG_V    	0x1UL


#define csr_read(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define csr_write(reg, val) ({ \
  asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

#endif /* CSR_H */
