#ifndef CONTEXT_H
#define CONTEXT_H

#include <type.h>

/* used to save register infomation */
typedef struct regs_context
{
	/* dasics supervisor registers */
	reg_t dasicsUmainCfg;  		/* initialize should be zero */
	reg_t dasicsUmainBoundLO;
	reg_t dasicsUmainBoundHI;

	/* dasics user registers */
	reg_t dasicsLibCfg0;
	reg_t dasicsLibBounds[32];

	reg_t dasicsMaincallEntry;
	reg_t dasicsReturnPC[4];
	reg_t dasicsFreeZoneReturnPC;	

	reg_t dasicsJmpBounds[8];
	reg_t dasicsJmpCfg;

	reg_t DasicsScratchCfg;
	reg_t DasicsScratchBound[2];
	reg_t DasicsMemLevel;
	reg_t DasicsJmpLevel;

    // N-extensions
	reg_t ustatus;
	reg_t uepc;
	reg_t ubadaddr;
	reg_t ucause;
	reg_t utvec;
	reg_t uie;
	reg_t uip;
	reg_t uscratch;

    /* Saved main processor registers.*/
    reg_t zero;
	reg_t ra;
	reg_t sp;
	reg_t gp;
	reg_t tp;
	reg_t t0;
	reg_t t1;
	reg_t t2;
	reg_t s0;
	reg_t s1;
	reg_t a0;
	reg_t a1;
	reg_t a2;
	reg_t a3;
	reg_t a4;
	reg_t a5;
	reg_t a6;
	reg_t a7;
	reg_t s2;
	reg_t s3;
	reg_t s4;
	reg_t s5;
	reg_t s6;
	reg_t s7;
	reg_t s8;
	reg_t s9;
	reg_t s10;
	reg_t s11;
	reg_t t3;
	reg_t t4;
	reg_t t5;
	reg_t t6;
    // reg_t regs[32];
    /* Saved special registers. */
    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
    reg_t sscratch;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context
{
    /* Callee saved registers.*/
    // reg_t regs[14];
    reg_t ra;
	reg_t sp;
    reg_t s0;
    reg_t s1;
    reg_t s2;
    reg_t s3;
    reg_t s4;
    reg_t s5;
    reg_t s6;
    reg_t s7;
    reg_t s8;
    reg_t s9;
    reg_t s10;
    reg_t s11;
    // /* satp reg */
    // reg_t satp;
} switchto_context_t;

#endif