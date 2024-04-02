#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>

#define SATP_MODE_SV39 8
#define SATP_MODE_SV48 9

#define SATP_ASID_SHIFT 44lu
#define SATP_MODE_SHIFT 60lu

#define NORMAL_PAGE_SHIFT 12lu
#define NORMAL_PAGE_SIZE (1lu << NORMAL_PAGE_SHIFT)
#define LARGE_PAGE_SHIFT 21lu
#define LARGE_PAGE_SIZE (1lu << LARGE_PAGE_SHIFT)
#define SATP_PPN_MASK ((1lu << SATP_ASID_SHIFT) - 1)

#define PTE_NUM (1lu << 9)

extern uint64_t _filesystem_start[];

#ifdef RAMFS
#define QEMU_DISK_OFFSET ((uint64_t)_filesystem_start)
#else 
#define QEMU_DISK_OFFSET NULL 
#endif


/*
 * Flush entire local TLB.  'sfence.vma' implicitly fences with the instruction
 * cache as well, so a 'fence.i' is not necessary.
 */
static inline void local_flush_tlb_all(void)
{
    __asm__ __volatile__ ("sfence.vma\nfence.i\nfence" : : : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__ ("sfence.vma %0\nfence.i\nfence" : : "r" (addr) : "memory");
}

static inline void local_flush_icache_all(void)
{
    asm volatile ("fence.i" ::: "memory");
}

static inline void local_flush_dcache_all(void)
{
    asm volatile ("fence" ::: "memory");
}

static inline void set_satp(
    unsigned mode, unsigned asid, unsigned long ppn)
{
    unsigned long __v =
        (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
    __asm__ __volatile__("sfence.vma\ncsrw satp, %0" : : "rK"(__v) : "memory");
}

static inline uint64_t get_pgdir()
{
	uint64_t satp; 
	asm volatile("csrr %0, satp" : "=r" (satp) );
	return (satp & SATP_PPN_MASK) << NORMAL_PAGE_SHIFT;
}

extern uintptr_t phyc_entry;

#define PGDIR_PA (phyc_entry + 0x800000)  // use bootblock's page as PGDIR
#define KERNEL_BASE 0xffffffc080200000

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_READ (1 << 1)     /* Readable */
#define _PAGE_WRITE (1 << 2)    /* Writable */
#define _PAGE_EXEC (1 << 3)     /* Executable */
#define _PAGE_USER (1 << 4)     /* User */
#define _PAGE_GLOBAL (1 << 5)   /* Global */
#define _PAGE_ACCESSED (1 << 6) /* Set by hardware on any access \
                                 */
#define _PAGE_DIRTY (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT (1 << 8)     /* Reserved for software */
#define _PAGE_SD   (1 << 9)     /* Clock SD flag */

#define _PAGE_PFN_SHIFT 10lu

#define VA_MASK ((1lu << 39) - 1)

#define PPN_BITS 9lu
#define NUM_PTE_ENTRY (1 << PPN_BITS)

typedef uint64_t PTE;

static inline uintptr_t kva2pa(uintptr_t kva)
{
    // TODO:
    /**
     * get the pa from kva 
     */
    /* mask == 0x0000003fffffffff */
    uint64_t mask = (uint64_t)(~0) >> 26;

    return (kva & mask) - (KERNEL_BASE & mask) + phyc_entry;
}

static inline uintptr_t pa2kva(uintptr_t pa)
{
    // TODO:
    /**
     * get pa from kva
     */
    /* mask == 0xffffffc00000000 */    
    return (pa - phyc_entry) + KERNEL_BASE;
}

static inline uint64_t get_pa(PTE entry)
{
    // TODO:
    /**
     * get pa from PTE
     */
    /* mask == 0031111111111111 */
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    /* ppn * 4096(0x1000) */
    return ((entry & mask) >> _PAGE_PFN_SHIFT) << 12;    
}

/* Get/Set page frame number of the `entry` */
static inline long get_pfn(PTE entry)
{
    // TODO:
    /**
     * get pfn
     */
    /*mask == 00c1111111111111*/
    uint64_t mask = (uint64_t)(~0) >> _PAGE_PFN_SHIFT;
    return (entry & mask) >> _PAGE_PFN_SHIFT; 
}

static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    // TODO:
    /* set pfn
     */
    *entry &= ((uint64_t)(~0) >> 54);
    *entry |= (pfn << _PAGE_PFN_SHIFT);
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry)
{
    // TODO:
    /**
     * get flag
     */

    uint64_t mask = (uint64_t)(~0) >> 54;
    return entry & mask;
}
static inline void set_attribute(PTE *entry, uint64_t bits)
{
    // TODO:
    /**
     * set flag
     */
    *entry &= ((uint64_t)(~0) << 10);
    *entry |= bits;
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    // TODO:
    /**
     * clear page
     */
    for(uint64_t i = 0; i < 0x1000; i+=8){
        *((uint64_t *)(pgdir_addr+i)) = 0;
    }
}

static inline PTE * get_PTE_of(uintptr_t va, uintptr_t pgdir_va){
    // A Tool
    /**
     * get the PTE of the va, only used for the user three level
     */
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    uint64_t node_flag = _PAGE_WRITE | _PAGE_READ | _PAGE_EXEC;
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    // first null
    if (page_base[vpn[2]] == NULL)
        return NULL;
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    if (get_attribute(page_base[vpn[2]]) & node_flag)
        return &page_base[vpn[0]];

    // second null
    if (second_page[vpn[1]] == NULL)
        return NULL;   
         
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 

    if (get_attribute(second_page[vpn[2]]) & node_flag)
        return &second_page[vpn[0]];    

    if (third_page[vpn[0]] == NULL || (third_page[vpn[0]] & _PAGE_PRESENT) == 0)
        return NULL;    
    return &third_page[vpn[0]];    
}

static inline uintptr_t get_kva_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO:
    /**
     * get kva from page
     */
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    uint64_t node_flag = _PAGE_WRITE | _PAGE_READ | _PAGE_EXEC;
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    /* first null */
    if (page_base[vpn[2]] == NULL)
        return NULL;
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    if (get_attribute(page_base[vpn[2]]) & node_flag)
        return pa2kva((get_pfn(page_base[vpn[2]]) << 30) | (va & ~(~((uint64_t)0) << 30)));;
    /* second null */
    if (second_page[vpn[1]] == NULL)
        return NULL;    
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
    if (get_attribute(second_page[vpn[1]]) & node_flag)
        return pa2kva((get_pfn(second_page[vpn[1]]) << LARGE_PAGE_SHIFT) | (va & ~(~((uint64_t)0) << LARGE_PAGE_SHIFT)));;    
    /* if null */
    if (third_page[vpn[0]] == NULL)
        return NULL;    
    return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT) | (va & ~(~((uint64_t)0) << NORMAL_PAGE_SHIFT)));
}

static inline uintptr_t get_pfn_of(uintptr_t va, uintptr_t pgdir_va){
    // TODO:
    /**
     * get table kva
     */
    uint64_t vpn[] = {
                      (va >> 12) & ~(~0 << 9), //vpn0
                      (va >> 21) & ~(~0 << 9), //vpn1
                      (va >> 30) & ~(~0 << 9)  //vpn2
                     };
    PTE *page_base;
    page_base = (PTE *)pgdir_va;
    // first null
    if (page_base[vpn[2]] == NULL)
        return NULL;
    PTE *second_page = (PTE *)pa2kva((get_pfn(page_base[vpn[2]]) << NORMAL_PAGE_SHIFT));
    // second null
    if (second_page[vpn[1]] == NULL)
        return NULL;    
    PTE *third_page  = (PTE *)pa2kva((get_pfn(second_page[vpn[1]]) << NORMAL_PAGE_SHIFT)); 
    /* third null */
    if (third_page[vpn[0]] == NULL)
        return NULL;    
    return pa2kva((get_pfn(third_page[vpn[0]]) << NORMAL_PAGE_SHIFT));    
}

#endif  // PGTABLE_H
