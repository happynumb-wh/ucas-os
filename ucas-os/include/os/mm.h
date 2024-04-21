#ifndef MM_H
#define MM_H

#include <os/spinlock.h>
#include <os/source.h>
#include <os/list.h>
#include <os/auxvec.h>
#include <type.h>
#include <pgtable.h>


extern uintptr_t __BSS_END__[];
/* used pagelist */
static LIST_HEAD(usedPageList);
/* shareMemKay */
static LIST_HEAD(shareMemKey);

#define KB 0x400lu
#define MB 0x100000lu
#define GB 0x40000000lu



#define MEM_SIZE 32
#define MAP_KERNEL 1
#define MAP_USER 2
#define LOAD 0
#define STORE 1
#define NO_ALLOC 0
#define ALLOC_NO_AD 1
#define IN_SD 2
#define IN_SD_NO_W 3
#define NO_W 4
#define DASICS_NO_V 5

/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))

#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))


#define MAX_PAGE_NUM 12

// Kernel mem will be 64 MB
#define KERNEL_END  (KERNEL_BASE + 512 * MB)

// Leave 32MB for the kernel bss_end
#define INIT_KERNEL_STACK ((uintptr_t)(__BSS_END__))
#define INIT_KERNEL_STACK_MSTER (INIT_KERNEL_STACK + PAGE_SIZE)
#define INIT_KERNEL_STACK_SLAVE (INIT_KERNEL_STACK_MSTER + PAGE_SIZE)

// pcb user stack message
#define USER_STACK_ADDR 0x3900000000lu
#define SIGNAL_HANDLER_ADDR 0x3800000000lu
#define USER_STACK_HIGN 0x3000000000lu
// alloc high 0.5MB for the Heap, never be realloc
// Free MEM
#define FREEMEM (INIT_KERNEL_STACK + 2 * PAGE_SIZE)
// the 6.5MB for the kernel
#define NUM_REC ((FREEHEAP - FREEMEM) / NORMAL_PAGE_SIZE)

#define FREEHEAP (KERNEL_END)

#define GET_MEM_NODE(baseAddr) (((baseAddr) - (FREEMEM)) / NORMAL_PAGE_SIZE)

#define MMAP_BASE 0x2100000000

extern uintptr_t boot_stack[PAGE_SIZE];
extern uintptr_t *__kzero_page;
extern void * __linker_buffer;

/* the struct for per page */
typedef struct page_node{
    list_node_t list;
    uint64_t kva_begin; /* the va of the page */
    uint8_t share_num;  /* share page number */
    uint8_t is_share;   /* maybe for the shm_pg_get */
}page_node_t;

typedef struct mm_struct
{
    uint64_t start_code;
    uint64_t end_code;
    uint64_t start_data;
    uint64_t end_data;
    uint64_t start_stack;       // wait
    uint64_t start_brk;         // wait
    uint64_t brk;
    uint64_t mmap_base;
    uint64_t elf_bss;
    uint64_t elf_brk;
    uint64_t ulibtext_start;
    uint64_t ulibtext_end;
    uint64_t text_start;
    uint64_t text_end;
    uint64_t ulibfreetext_start;
    uint64_t ulibfreetext_end;
    uint64_t ulib_sec_exist;
    uint64_t free_sec_exit;
    uint64_t map_now;
    uint64_t aux_elem_t[AT_VECTOR_SIZE];
} mm_struct_t;



/* manager free page */
extern source_manager_t freePageManager;



extern ptr_t memCurr;

extern page_node_t *pageRecyc;

extern uint32_t init_mem();
extern uint64_t krand();
/* dynamic malloc */
extern ptr_t allocPage();
extern void freePage(ptr_t baseAddr);
extern void freePage_voilence(ptr_t baseAddr);

extern void* kmalloc(size_t size);
extern void kfree(void * size);
extern void kfree_voilence(void * size);

extern void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir);
/* alloc a new page */
extern void * alloc_page_helper(uintptr_t va, uintptr_t pgdir, uint64_t mode, uint64_t flag);
/* alloc a page pointer phyc */
extern PTE * alloc_page_point_phyc(uintptr_t va, uintptr_t pgdir, uint64_t kva, uint64_t mode, uint64_t flag);

/* check weather the va has been used*/
// extern uint32_t check_page_map(uintptr_t vta, uintptr_t pgdir);
/* check the error type */
extern uint32_t check_W_SD_and_set_AD(uintptr_t va, uintptr_t pgdir, int mode);

/* share mem get */
uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);
/* check the page and set flag for it */
PTE check_page_set_flag(PTE* page, uint64_t vpn, uint64_t flag, int bzero);

/* free pgtable */
uintptr_t free_page_helper(uintptr_t va, uintptr_t pgdir);
/* recycle */
int recycle_page_default(uintptr_t pgdir);
int recycle_page_voilent(uintptr_t pgdir);
int recycle_page_part(uintptr_t pgdir, void *recyclePcb);

/* change the data  */
uint64_t do_brk(uintptr_t ptr);
uint64_t lazy_brk(uintptr_t ptr);
int do_mprotect(void *addr, size_t len, int prot);
int do_madvise(void* addr, size_t len, int notice);
int do_membarrier(int cmd, int flags);
/* deal the mmirq with clone no W set */
void deal_no_W(uint64_t fault_addr);
/* alloc_page_helper addr ok */ 
uint8 is_legal_addr(uint64_t fault_addr);
void share_add(uint64_t baseAddr);
void share_sub(uint64_t baseAddr);
// membarrier
int do_membarrier(int cmd, int flags);
#endif /* MM_H */
