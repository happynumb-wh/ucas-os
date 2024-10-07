#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/elf.h>
#include <pgtable.h>

#include <assert.h>

/* recycle */
int recycle_page_default(uintptr_t pgdir){
    int recyclePageNum = 0;
    PTE *recycleBase = (PTE *)pgdir;
    for (int vpn2 = 0; vpn2 < PTE_NUM / 2; vpn2++)
    {
        if (!recycleBase[vpn2]) 
            continue;
        PTE *second_page = (PTE *)pa2kva((get_pfn(recycleBase[vpn2]) << NORMAL_PAGE_SHIFT));
        /* we will let the user be three levels page */ 
        for (int vpn1 = 0; vpn1 < PTE_NUM; vpn1++)
        {
            if (!second_page[vpn1]) 
                continue;
            PTE *third_page = (PTE *)pa2kva((get_pfn(second_page[vpn1]) << NORMAL_PAGE_SHIFT));
            for (int vpn0 = 0; vpn0 < PTE_NUM; vpn0++)
            {
                if (!third_page[vpn0])
                    continue;
                uintptr_t final_page = pa2kva((get_pfn(third_page[vpn0]) << NORMAL_PAGE_SHIFT));
                freePage(final_page);
                recyclePageNum++;
            }
            freePage((uintptr_t)third_page);
            recyclePageNum++;
        }
        freePage((uintptr_t)second_page);       
        recyclePageNum++;
    }
    /* change the pgdir to kernel */
    set_satp(SATP_MODE_SV39, 0, \
        (uint64_t)kva2pa(PGDIR_PA) >> 12);
    local_flush_tlb_all();
    freePage((uintptr_t)recycleBase);

    recyclePageNum++;
    return recyclePageNum;
}

/* recycle */
int recycle_page_voilent(uintptr_t pgdir){
    int recyclePageNum = 0;
    PTE *kernelBase = (PTE *)PGDIR_PA;
    PTE *recycleBase = (PTE *)pgdir;
    for (int vpn2 = 0; vpn2 < PTE_NUM; vpn2++)
    {
        if (kernelBase[vpn2] || !recycleBase[vpn2]) 
            continue;
        PTE *second_page = (PTE *)pa2kva((get_pfn(recycleBase[vpn2]) << NORMAL_PAGE_SHIFT));
        /* we will let the user be three levels page */ 
        for (int vpn1 = 0; vpn1 < PTE_NUM; vpn1++)
        {
            if (!second_page[vpn1]) 
                continue;
            PTE *third_page = (PTE *)pa2kva((get_pfn(second_page[vpn1]) << NORMAL_PAGE_SHIFT));
            for (int vpn0 = 0; vpn0 < PTE_NUM; vpn0++)
            {
                if (!third_page[vpn0])
                    continue;
                uintptr_t final_page = pa2kva((get_pfn(third_page[vpn0]) << NORMAL_PAGE_SHIFT));
                freePage(final_page);
                recyclePageNum++;
            }
            freePage((uintptr_t)third_page);
            recyclePageNum++;
        }
        freePage((uintptr_t)second_page);        
        recyclePageNum++;
    }
    /* change the pgdir to kernel */
    set_satp(SATP_MODE_SV39, 0, \
        (uint64_t)kva2pa(PGDIR_PA) >> 12);
    local_flush_tlb_all();    
    freePage((uintptr_t)recycleBase);
    recyclePageNum++;
    return recyclePageNum;
}

/**
 * @brief recycle the data phdr
 * @param pgdir the recycle pab pgdir
 * @param recyclePcb the recycle pcb
 * @return the recycle page num
 * 
 */
int recycle_page_part(uintptr_t pgdir, void *exe_load)
{
    // // printk("%s recycle\n", current_running->name);
    // assert(exe_load != NULL);
    // excellent_load_t* recycle_load = (excellent_load_t *)exe_load;
    // int recyclePageNum = 0;
    // PTE *kernelBase = (PTE *)PGDIR_PA;
    // PTE *recycleBase = (PTE *)pgdir;
    // uintptr_t keep_vaddr_base = recycle_load->phdr[0].p_vaddr;
    // uintptr_t keep_vaddr_top = recycle_load->phdr[0].p_vaddr + 
    //                                 recycle_load->phdr[0].p_memsz;
    // uintptr_t keep_dy_vaddr_base;
    // uintptr_t keep_dy_vaddr_top;
    
    // if (recycle_load->dynamic)
    // {
    //     int id = find_name_elf("libc.so");
    //     assert(id != -1);
    //     excellent_load_t* dynamic_load = &pre_elf[id];
    //     keep_dy_vaddr_base = dynamic_load->phdr[0].p_vaddr + DYNAMIC_VADDR_PFFSET;
    //     keep_dy_vaddr_top = dynamic_load->phdr[0].p_vaddr + 
    //                         dynamic_load->phdr[0].p_memsz + DYNAMIC_VADDR_PFFSET;        
    // }
    // for (int vpn2 = 0; vpn2 < PTE_NUM; vpn2++)
    // {
    //     if (kernelBase[vpn2] || !recycleBase[vpn2]) 
    //         continue;
    //     PTE *second_page = (PTE *)pa2kva((get_pfn(recycleBase[vpn2]) << NORMAL_PAGE_SHIFT));
    //     if (!(second_page >= KERNEL_ENTRYPOINT && second_page <= KERNEL_END)){
    //         // printk("[recycle] error second_page 0x%lx!\n",second_page);
    //         continue;
    //         //while(1);
    //     }
    //     /* we will let the user be three levels page */ 
    //     for (int vpn1 = 0; vpn1 < PTE_NUM; vpn1++)
    //     {
    //         if (!second_page[vpn1]) 
    //             continue;
    //         PTE *third_page = (PTE *)pa2kva((get_pfn(second_page[vpn1]) << NORMAL_PAGE_SHIFT));
    //         if (!(third_page >= KERNEL_ENTRYPOINT && third_page <= KERNEL_END)){
    //             // printk("[recycle] error third_page 0x%lx!\n",third_page);
    //             continue;
    //             //while(1);
    //          }
    //         for (int vpn0 = 0; vpn0 < PTE_NUM; vpn0++)
    //         {
    //             if (!third_page[vpn0])
    //                 continue;
    //             uintptr_t final_page = pa2kva((get_pfn(third_page[vpn0]) << NORMAL_PAGE_SHIFT));
    //             if (!(final_page >= KERNEL_ENTRYPOINT && final_page <= KERNEL_END)){
    //                 // printk("[recycle] error final_page 0x%lx!\n",final_page);
    //                 continue;
    //                 //while(1);
    //             }    
    //             uint64_t vaddr = ((uint64_t)vpn0 << 12) | 
    //                              ((uint64_t)vpn1 << 21) |
    //                              ((uint64_t)vpn2 << 30);  
    //             // if (!recycle_load->dynamic)
    //                 if (vaddr >= keep_vaddr_base && vaddr <= keep_vaddr_top)
    //                 {
    //                     continue;   
    //                 }
    //             // for libc.so
    //             if (recycle_load->dynamic)
    //             {
    //                 if (vaddr >= keep_dy_vaddr_base && vaddr <= keep_dy_vaddr_top)
    //                 {
    //                     continue;   
    //                 }                    
    //             }
    //             // printk("recycle : %lx\n", vaddr);
                 
    //             freePage(final_page);
    //             recyclePageNum++;
    //         }
    //         freePage((uintptr_t)third_page);
    //         recyclePageNum++;
    //     }
    //     freePage((uintptr_t)second_page);        
    //     recyclePageNum++;
    // }
    // /* change the pgdir to kernel */
    
    // // if (current_running->pgdir == pgdir)
    // // {
    // //     set_satp(SATP_MODE_SV39, 0, kva2pa(PGDIR_PA) >> NORMAL_PAGE_SHIFT);
    // //     local_flush_tlb_all();        
    // // }
    // freePage((uintptr_t)recycleBase);
    // recyclePageNum++;
    // return recyclePageNum;
    return 0;
}