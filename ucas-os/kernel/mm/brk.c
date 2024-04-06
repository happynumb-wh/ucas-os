#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <pgtable.h>
#include <os/string.h>
#include <stdio.h>
#include <assert.h>

/**
 * @brief change the edata 
 * @param ptr specify the size to be modified
 * @return succeed return 0 else return -1
 */
uint64_t do_brk(uintptr_t ptr)
{
    
    if (ptr > current_running->user_stack_base) 
    {
        printk("the ptr: %lx lager than the user stack base: %lx\n", ptr, current_running->user_stack_base);
        return -ENOMEM;
    } else if (ptr == 0) {
        return current_running->edata;
    }
    else if (ptr < current_running->edata) 
    {
        for (uintptr_t cur_page = ptr; cur_page < current_running->edata; cur_page += NORMAL_PAGE_SIZE){
            if (get_kva_of(cur_page,current_running->pgdir) != 0){
                free_page_helper(cur_page, current_running->pgdir);
                local_flush_tlb_page(cur_page);
            }     
        }
        current_running->edata = ptr;
        return current_running->edata;
    } else
    {
        for (uintptr_t my_ptr = current_running->edata; my_ptr < ptr; my_ptr += NORMAL_PAGE_SIZE)
        {
            alloc_page_helper(my_ptr, current_running->pgdir, MAP_USER, _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC);
            local_flush_tlb_page(my_ptr);
        }
        current_running->edata = ptr;
        return current_running->edata;
    }
}

uint64_t lazy_brk(uintptr_t ptr)
{
    
    if (ptr > current_running->user_stack_base) 
    {
        printk("the ptr: %lx lager than the user stack base: %lx\n", ptr, current_running->user_stack_base);
        return -ENOMEM;
    } else if (ptr == 0) {
        return current_running->edata;
    }
    else if (ptr < current_running->edata) 
    {
        printk("the ptr: %lx less than the now edata: %lx\n", ptr, current_running->edata);
        return -EINVAL;
    } else
    {
        for (uintptr_t my_ptr = current_running->edata; my_ptr < ptr; my_ptr += NORMAL_PAGE_SIZE)
        {
            // alloc_page_helper(my_ptr, current_running->pgdir, MAP_USER);
        }
        current_running->edata = ptr;
        return current_running->edata;
    }
}

int do_mprotect(void *addr, size_t len, int prot){
    // printk("do_mprotect need to do.\n");
    

    return 0;
}
int do_madvise(void* addr, size_t len, int notice){
    // printk("do_madvise need to do.\n");
    // seems that there is no need to imply this (used in getpwnam test)
    return 0;
}
int do_membarrier(int cmd, int flags){
    // printk("[do_membarrier] cmd = 0x%x, flags = 0x%x\n",cmd,flags);
    return 0;
}
