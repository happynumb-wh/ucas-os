#include <os/kdasics.h>
#include <context.h>
#include <stdio.h>



void handle_dasics_fetch_fault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{

    printk("[DASICS EXCEPTION]Info: dasics fetch fault occurs, scause = 0x%lx, sepc = 0x%lx, stval = 0x%lx\n",\
     regs->scause, regs->sepc, regs->sbadaddr);

    // jump the ins
    regs->sepc += 4;

    return;
}

void handle_dasics_load_fault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
   
    printk("[DASICS EXCEPTION]Info: dasics load fault occurs, scause = 0x%lx, sepc = 0x%lx, stval = 0x%lx\n", \
            regs->scause, regs->sepc, regs->sbadaddr);

    // jump the ins
    regs->sepc += 4; 

    return;
}

void handle_dasics_store_fault(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
  
    printk("[DASICS EXCEPTION]Info: dasics store fault occurs, scause = 0x%lx, sepc = 0x%lx, stval = 0x%lx\n", \
            regs->scause, regs->sepc, regs->sbadaddr);
    // jump the ins
    regs->sepc += 4;     
    return;
}
