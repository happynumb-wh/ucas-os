#ifndef _INCLUDE_OS_DASICS_H
#define _INCLUDE_OS_DASICS_H

#include <os/smp.h>
#include <type.h>
#include <csr.h>

typedef struct pcb pcb_t; // define in <os/sched.h>


// dasics elf type
#define NO_DASICS 0
#define DASICS_STATIC 1
#define DASICS_DYNAMIC 2

// judge the dasics option
#define DASICS_COMMAND "-dasics"
#define DASICS_LENGTH 8

// dasics dynamic elf base
#define DASICS_LINKER_BASE 0x1000 
#define COPY_LINKER_BASE 0x30000
#define TRUST_LIB_BASE 0x800000


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