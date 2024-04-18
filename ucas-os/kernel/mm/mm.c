#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/string.h>
#include <pgtable.h>
#include <assert.h>

ptr_t memCurr = FREEMEM;
ptr_t heapCurr = FREEHEAP;

// memory managemanet
page_node_t * pageRecyc = NULL;
// control of the free page
source_manager_t freePageManager;

// When boot loader，this page will be used as boot stack
// After boot, this page will be used as zero page
uintptr_t boot_stack[PAGE_SIZE] __aligned(PAGE_SIZE);
uintptr_t *__kzero_page = NULL; 

// for fat32_load_elf
uint64_t kload_buffer;
void * __linker_buffer;


uint64_t krand()
{
    uint64_t rand_base = get_ticks();
    long long tmp = 0x5deece66dll * rand_base + 0xbll;
    uint64_t result = tmp & 0x7fffffff;
    return result;
}

// Init the zero page
static void __init_kzero_page()
{
    __kzero_page = (void *)(allocPage() - PAGE_SIZE);

    memset(__kzero_page, 0 , PAGE_SIZE);

}

// init mem
uint32_t init_mem(){
    pageRecyc = (page_node_t *)kmalloc(sizeof(page_node_t) * NUM_REC + PAGE_SIZE * 2);
    kload_buffer = (uint64_t)kmalloc(MB * 24);
    __linker_buffer = (void* )kmalloc(MB * 4);

    uint64_t memBegin = memCurr;
    // init list
    init_list(&freePageManager.free_source_list);
    init_list(&freePageManager.wait_list);

    for (int i = 0; i < NUM_REC; i++){
        list_add_tail(&pageRecyc[i].list, &freePageManager.free_source_list);
        pageRecyc[i].kva_begin = memBegin;
        pageRecyc[i].share_num = 0;
        pageRecyc[i].is_share = 0;
        memBegin += NORMAL_PAGE_SIZE;
    }

    freePageManager.source_num = NUM_REC;

    __init_kzero_page();

    return NUM_REC;
}

// alloc one page, return the top of the page
ptr_t allocPage() 
{
    list_head *freePageList = &freePageManager.free_source_list;
    
try:    if (!list_empty(freePageList)){
        list_node_t* new_page = freePageList->next;
        page_node_t *new = list_entry(new_page, page_node_t, list);
        /* one use */
        new->share_num++;
        list_del(new_page);
        list_add_tail(new_page, &usedPageList);

        freePageManager.source_num--;
        // printk("alloc mem kva 0x%lx\n", new->kva_begin);
        return new->kva_begin + PAGE_SIZE;        
    }else{
        printk("Failed to allocPage, I am pid: %d\n", current_running->pid);
        do_block(&current_running->list, &freePageManager.wait_list);
        goto try;
    }
}

// free one page, get the base addr of the page
void freePage(ptr_t baseAddr)
{
    // printk("freePage:%lx\n",baseAddr);
    /* alloc page */
    page_node_t *recycle_page = &pageRecyc[GET_MEM_NODE(baseAddr)];
    
    /* the page has been shared */
    if (--(recycle_page->share_num) != 0) {
        return;
    }
    if (baseAddr == (uintptr_t)__kzero_page) return;

    list_head *freePageList = &freePageManager.free_source_list;    
    list_del(&recycle_page->list);
    list_add(&recycle_page->list, freePageList);
    freePageManager.source_num++;
    // we only free one
    if (!list_empty(&freePageManager.wait_list))
        do_unblock(freePageManager.wait_list.next);
}

// don't care the 
void freePage_voilence(ptr_t baseAddr)
{
    /* alloc page */
    page_node_t *recycle_page = &pageRecyc[GET_MEM_NODE(baseAddr)];
    recycle_page->share_num = 0;
    list_head *freePageList = &freePageManager.free_source_list;    
    list_del(&recycle_page->list);
    list_add_tail(&recycle_page->list, freePageList);
    freePageManager.source_num++;
    if (!list_empty(&freePageManager.wait_list))
        do_unblock(freePageManager.wait_list.next);
}

/* malloc a place in the mem, return a place */
void *kmalloc(size_t size)
{
    if (size != NORMAL_PAGE_SIZE) {
        ptr_t ret = ROUND(heapCurr, 8);
        heapCurr = ret + size;        
        return (void*)ret;
    }
    else {
        return (void*)(allocPage() - NORMAL_PAGE_SIZE);
    } 
}

/* free a mem */
void kfree(void * size){
    if ((ptr_t)size > FREEHEAP) ; // nrver recycle
    else if ((ptr_t)size < FREEMEM) ;// null or less
    else {
        freePage((ptr_t)size);
    }
}

/*  */
extern void kfree_voilence(void * size)
{
    if ((ptr_t)size > FREEHEAP) ; // nrver recycle
    else if ((ptr_t)size < FREEMEM) ;// null or less
    else {
        freePage_voilence((ptr_t)size);
    }   
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    memcpy((char *)dest_pgdir, (char *)src_pgdir, PAGE_SIZE);
}

/**
 * @brief Check whether the page exists
 * if not , alloc it and set the flag
 * return the pagebase
 */
PTE check_page_set_flag(PTE* page, uint64_t vpn, uint64_t flag, int bzero){
    if (page[vpn] & _PAGE_PRESENT) 
    {
        /* valid */
        return pa2kva((get_pfn(page[vpn]) << NORMAL_PAGE_SHIFT));
    } else
    {
        uint64_t newpage = allocPage() - PAGE_SIZE;
        if (bzero)
        {
            /* clear the second page */
            clear_pgdir(newpage);            
        }
        set_pfn(&page[vpn], kva2pa(newpage) >> NORMAL_PAGE_SHIFT);
        /* maybe need to set the U, the kernel will not set the U 
         * which means that the User will not get it, but we will 
         * temporary set it as the User. so the U will be high
        */ 
        set_attribute(&page[vpn], flag);
        
        return newpage; 
    }

}

/**
 * @brief for the clone to add one more share the phyc page
 * no return;
 */
void share_add(uint64_t baseAddr){
    pageRecyc[GET_MEM_NODE(baseAddr)].share_num++;
    return;
}
/**
 * @brief for the clone share_num -- 
 * no return 
 */
void share_sub(uint64_t baseAddr){
    pageRecyc[GET_MEM_NODE(baseAddr)].share_num--;
    return;  
}


/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
void * alloc_page_helper(uintptr_t va, uintptr_t pgdir, uint64_t mode, uint64_t flag)
{
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    
    second_page = (PTE *)check_page_set_flag(page_base, vpn[2], _PAGE_PRESENT, 1);
    /* third page */
    third_page = (PTE *)check_page_set_flag(second_page, vpn[1], _PAGE_PRESENT, 1);

    /* final page */
    /* the R,W,X == 1 will be the leaf */
    // uint64_t pte_flags = _PAGE_PRESENT | _PAGE_READ  | _PAGE_WRITE    
    //                     | _PAGE_EXEC  | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
    //                       _PAGE_USER); 
#ifdef DASICS_DEBUG_EXCEPTION
    uint64_t pte_flags = flag
                          | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
                          (_PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY) );

#else

    uint64_t pte_flags = _PAGE_PRESENT | flag
                          | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
                          (_PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY) );
#endif

    /* final page */
    return (void *)(check_page_set_flag(third_page, vpn[0], pte_flags, 0) | (va & (uint64_t)0x0fff));

}

/* alooc the va to the pointer */
PTE * alloc_page_point_phyc(uintptr_t va, uintptr_t pgdir, uint64_t kva, uint64_t mode, uint64_t flag){
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    second_page = (PTE *)check_page_set_flag(page_base, vpn[2], _PAGE_PRESENT, 1);

    /* third page */
    third_page = (PTE *)check_page_set_flag(second_page, vpn[1], _PAGE_PRESENT, 1);
    /* final page */
    // if((third_page[vpn[0]] & _PAGE_PRESENT) != 0)
    //     /* return the physic page addr */
    //     return 0;//pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));

    /* the physical page */
    third_page[vpn[0]] = 0;
    set_pfn(&third_page[vpn[0]], kva2pa(kva) >> NORMAL_PAGE_SHIFT);
    /* maybe need to assign U to low */
    // Generate flags
    /* the R,W,X == 1 will be the leaf */
    // uint64_t pte_flags = _PAGE_PRESENT | _PAGE_READ  | _PAGE_WRITE    
    //                     | _PAGE_EXEC  | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
    //                       _PAGE_USER);
    if (kva > (uint64_t)__BSS_END__)
        pageRecyc[GET_MEM_NODE(kva)].share_num ++;

#ifdef DASICS_DEBUG_EXCEPTION
    uint64_t pte_flags = flag
                          | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
                          (_PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY));

#else
    uint64_t pte_flags = _PAGE_PRESENT | flag
                          | (mode == MAP_KERNEL ? (_PAGE_ACCESSED | _PAGE_DIRTY) :
                          (_PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY));
#endif

    set_attribute(&third_page[vpn[0]], pte_flags);
    return &third_page[vpn[0]];     
}

// uint32_t check_page_map(uintptr_t vta, uintptr_t pgdir){
//     uint64_t vpn[] = {
//                     (vta >> 12) & ~(~0 << 9), //vpn0
//                     (vta >> 21) & ~(~0 << 9), //vpn1
//                     (vta >> 30) & ~(~0 << 9)  //vpn2
//                     };
//     /* the PTE in the first page_table */
//     PTE *page_base = (uint64_t *) pgdir;
//     /* second page */
//     PTE *second_page = NULL;
//     /* finally page */
//     PTE *third_page = NULL;
//     /* find the second page */
//     if (((page_base[vpn[2]]) & _PAGE_PRESENT) == 0)//unvalid
//     {
//         return 1;
//     }
//     else
//     {
//         /* get the addr of the second_page */
//         second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
//     }
    
//     /* third page */
//     if (((second_page[vpn[1]]) & _PAGE_PRESENT) == 0 )//unvalid or the leaf
//     {
//         return 1;
//     }
//     else
//     {
//         third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
//         /* the va's page */
//     }

//     /* final page */
//     if((third_page[vpn[0]] & _PAGE_PRESENT) == 0)
//         return 1;
//     else
//         return 0;
// }

/* check the physic page in only without AD or in the SD */
uint32_t check_W_SD_and_set_AD(uintptr_t va, uintptr_t pgdir, int mode){
    uint64_t vpn[] = {
                    (va >> 12) & ~(~0 << 9), //vpn0
                    (va >> 21) & ~(~0 << 9), //vpn1
                    (va >> 30) & ~(~0 << 9)  //vpn2
                    };
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL; 
    if (((page_base[vpn[2]]) & _PAGE_PRESENT)!=0)
    {
        second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    }    
    else
        return NO_ALLOC;

    if (((second_page[vpn[1]]) & _PAGE_PRESENT)!=0)
    {
        third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));
    }    
    else
        return NO_ALLOC; 


    if (((third_page[vpn[0]]) & _PAGE_PRESENT)!=0)
    {
        third_page[vpn[0]] |= (_PAGE_ACCESSED | (mode == STORE ? _PAGE_DIRTY: 0));

        if((third_page[vpn[0]] & _PAGE_WRITE) == 0 && mode == STORE){
            return NO_W;
        }
    }    
    else{
        if (third_page[vpn[0]] != 0)
            return DASICS_NO_V;
        return NO_ALLOC;
    }
    return ALLOC_NO_AD;
}

uintptr_t free_page_helper(uintptr_t va, uintptr_t pgdir)
{
    
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    /* the PTE in the first page_table */
    PTE *page_base = (uint64_t *) pgdir;
    /* second page */
    PTE *second_page = NULL;
    /* finally page */
    PTE *third_page = NULL;
    /* find the second page */
    if((page_base[vpn[2]] & _PAGE_PRESENT) == 0){
        return -1;
    }
    second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    if((second_page[vpn[1]] & _PAGE_PRESENT) == 0){
        return -1;
    }
    /* third page */
    third_page = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT));

    if (third_page[vpn[0]])
    {
        freePage(pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT)));
        third_page[vpn[0]] = 0;
    }

    return 0;
}