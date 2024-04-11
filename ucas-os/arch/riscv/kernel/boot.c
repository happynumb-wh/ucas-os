/* RISC-V kernel boot stage */
#include <pgtable.h>
#include <asm.h>

typedef void (*kernel_entry_t)(unsigned long);
uintptr_t phyc_entry  = 0;

uintptr_t kernel_pgd[PAGE_SIZE / RISCV_SZPTR] __page_aligned_bss;
uintptr_t kernel_pmd[PAGE_SIZE / RISCV_SZPTR] __page_aligned_bss;
uintptr_t boot_pmd[PAGE_SIZE / RISCV_SZPTR] __page_aligned_bss;


// using 2MB large page
static void __init map_page(int boot, uint64_t va, uint64_t pa, PTE *pgdir)
{
    va &= VA_MASK;
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    if (pgdir[vpn2] == 0) {
        // alloc a new second-level page directory
        set_pfn(&pgdir[vpn2], boot == BOOT_PAGE ? (uintptr_t)boot_pmd >> NORMAL_PAGE_SHIFT :\
                                 (uintptr_t)kernel_pmd >> NORMAL_PAGE_SHIFT);
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        clear_pgdir(get_pa(pgdir[vpn2]));
    }
    PTE *pmd = (PTE *)get_pa(pgdir[vpn2]);
    set_pfn(&pmd[vpn1], pa >> NORMAL_PAGE_SHIFT);
    set_attribute(
        &pmd[vpn1], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                        _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY | _PAGE_GLOBAL);
}

static void __init enable_vm()
{
    // write satp to enable paging
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
}

/* Sv-39 mode
 * 0x0000_0000_0000_0000-0x0000_003f_ffff_ffff is for user mode
 * 0xffff_ffc0_0000_0000-0xffff_ffff_ffff_ffff is for kernel mode
 */
static void __init setup_vm(uintptr_t entry)
{
    clear_pgdir(PGDIR_PA);
    // map kernel virtual address(kva) to kernel physical
    // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
    // map all physical memory
    PTE *early_pgdir = (PTE *)PGDIR_PA;
    for (uint64_t kva = 0xffffffc080000000lu;
         kva < 0xffffffc0c0000000lu; kva += 0x200000lu) {
        map_page(KERNEL_PAGE, kva, kva2pa(kva), early_pgdir);
    }
    // map boot address
    for (uint64_t pa = phyc_entry; pa < (uintptr_t)_end;
         pa += 0x200000lu) {
        map_page(BOOT_PAGE, pa, pa, early_pgdir);
    }
    enable_vm();
}

extern uintptr_t _start[];
extern uintptr_t _boot[];

/*********** start here **************/
int __init boot_kernel(unsigned long mhartid, unsigned long fdt)
{
    if (mhartid == 0) {
        // Set the physic entry point
        phyc_entry = (uintptr_t)_boot;
        setup_vm((uintptr_t)_boot);
    } else {
        enable_vm();
    }

    /* go to kernel */
    ((kernel_entry_t)pa2kva((uintptr_t)_start))(mhartid);

    return 0;
}
