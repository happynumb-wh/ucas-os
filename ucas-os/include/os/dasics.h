#ifndef _INCLUDE_OS_DASICS_H
#define _INCLUDE_OS_DASICS_H

#include <os/smp.h>
#include <type.h>
#include <hash/uthash.h>
#include <csr.h>

typedef struct pcb pcb_t; // define in <os/sched.h>

#define BOUND_REG_READ(PRO,hi,lo,idx)   \
        case idx:  \
            lo = (pcb_t *)PRO->save_context->dasicsLibBounds[idx * 2]; \
            hi = (pcb_t *)PRO->save_context->dasicsLibBounds[idx * 2 + 1]; \
            break;

#define BOUND_REG_WRITE(PRO,hi,lo,idx)   \
        case idx:  \
            (pcb_t *)PRO->save_context->dasicsLibBounds[idx * 2] = lo; \
            (pcb_t *)PRO->save_context->dasicsLibBounds[idx * 2 + 1] = hi; \
            break;

#define CONCAT(OP) BOUND_REG_##OP

#define LIBBOUND_LOOKUP(PRO,HI,LO,IDX,OP) \
        switch (IDX) \
        {               \
            CONCAT(OP)(PRO,HI,LO,0);  \
            CONCAT(OP)(PRO,HI,LO,1);  \
            CONCAT(OP)(PRO,HI,LO,2);  \
            CONCAT(OP)(PRO,HI,LO,3);  \
            default: \
                printk("\x1b[31m%s\x1b[0m","[DASICS]Error: out of libound register range\n"); \
        }


typedef struct {
    uint64_t lo;
    uint64_t hi;
} bound_t;

typedef struct hash_bound {
    int handle;  /* key */
    int priv;
    bound_t bound;
    UT_hash_handle hh;
} hashed_bound_t;


enum DasicsExCode
{
    FDIUJumpFault           = 24,
    FDIULoadAccessFault     = 25,
    FDIUStoreAccessFault    = 26
};

typedef struct regs_context regs_context_t;

void handle_dasics_fetch_fault(regs_context_t *regs, uint64_t stval, uint64_t cause);
void handle_dasics_load_fault(regs_context_t *regs, uint64_t stval, uint64_t cause);
void handle_dasics_store_fault(regs_context_t *regs, uint64_t stval, uint64_t cause);






















#endif