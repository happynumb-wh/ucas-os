#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <pgtable.h>
#include <os/string.h>
#include <assert.h>

/**
 * @brief for the clone copy on write 
 * 
 */
void deal_no_W(uint64_t fault_addr)
{
    // printk("deal no W: 0x%lx\n", fault_addr);
    
    /* the fault pte */
    PTE *fault_pte = get_PTE_of(fault_addr, current_running->pgdir);
    /* fault kva */
    uint64_t fault_kva = pa2kva((get_pfn(*fault_pte) << NORMAL_PAGE_SHIFT));
    
    // dont't care kzero_page
    if (fault_kva != (uint64_t)__kzero_page &&\
         pageRecyc[GET_MEM_NODE(fault_kva)].share_num == 1) {
        set_attribute(fault_pte, get_attribute(*fault_pte) | _PAGE_WRITE);
        return;
    }
    /* share_num --  */
    share_sub(fault_kva);    

    // release(&memLock);

    /* alloc a new page for current */
    uintptr_t new_page = allocPage() - PAGE_SIZE;
    /* copy on write */
    share_pgtable(new_page, fault_kva);
    /* set pfn */
    set_pfn(fault_pte, kva2pa(new_page) >> NORMAL_PAGE_SHIFT);
    /* set attribute */
    set_attribute(fault_pte, get_attribute(*fault_pte) | _PAGE_WRITE);
}   

/**
 * @brief judge the addr is legal addr
 * @param fault_addr the addr exception
 * @return if legal return 1 else return 0
 */
uint8 is_legal_addr(uint64_t fault_addr)
{
    
    // for heap
    if (fault_addr < current_running->edata && \
        fault_addr > current_running->elf.edata)
        return 1;

    // for stack
    uint64_t __maybe_unused sp = current_running->save_context->sp;
    if (fault_addr & USER_STACK_HIGN)
        return 1;
    return 0;
}