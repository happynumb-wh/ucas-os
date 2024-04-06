#include <os/elf.h>
#include <os/mm.h>
#include <os/sched.h>
#include <stdio.h>
#include <compile.h>
#include <user_programs.h>



char * dll_linker = "/lib/ld-linux-riscv64-lp64d.so.1";
// load the connect into the memory
extern uintptr_t load_connector(const char * filename, uintptr_t pgdir)
{
    int elf_id;
    while ((elf_id = find_name_elf(filename)) == -1)
    {
        printk("[Error] fast_load_elf failed to find %s\n", filename);
        do_pre_load(dll_linker, ELF_LOAD);
        // return 0;
    }   
    
    // the exe_load
    excellent_load_t * exe_load = &pre_elf[elf_id];
    uint64_t buffer_begin;
    list_node_t * load_node = exe_load->list.next;
    for (int index = 0; index < exe_load->phdr_num; index++)
    {
        uintptr_t dynamic_va_base = exe_load->phdr[index].p_vaddr + DYNAMIC_VADDR_PFFSET;
        uint64_t page_flag = get_page_flag(&exe_load->phdr[index]);
        for (int i = 0; i < exe_load->phdr[index].p_memsz; i += NORMAL_PAGE_SIZE)
        {
for_page_remain: ;
            uintptr_t v_base = dynamic_va_base + i;
            if (i < exe_load->phdr[index].p_filesz)
            {
                // contious
                buffer_begin  = list_entry(load_node, page_node_t, list)->kva_begin;  
                load_node = load_node->next;
                 
                uint64_t page_top = ROUND(v_base, 0x1000);
                uint64_t page_remain = (uint64_t)page_top - (uint64_t)(v_base); 
                // printk("vaddr: 0x%lx, page_top: 0x%lx, page_remain: 0x%lx\n",
                //         exe_load->phdr[index].p_vaddr + i, page_top, page_remain);
                unsigned char *bytes_of_page;
                // load
                if (page_remain == 0) 
                {
                    // if (!(exe_load->phdr[index].p_flags & PF_W) && !exe_load->dynamic)
                    // {
                    //     alloc_page_point_phyc(v_base, pgdir, buffer_begin, MAP_USER);
                    //     bytes_of_page = (char *)buffer_begin;
                    // } else 
                    // {
                        // this will never happen for the first phdr
                        // else alloc and copy
                        bytes_of_page = (void *)alloc_page_helper(v_base, pgdir, MAP_USER, page_flag);
                        memcpy(
                            bytes_of_page,
                            (void *)buffer_begin,
                            MIN(exe_load->phdr[index].p_filesz - i, NORMAL_PAGE_SIZE));
                    // }
                } else 
                {
                    // if not align, we can make sure it will be the data phdr
                    bytes_of_page = alloc_page_helper(v_base, pgdir, MAP_USER, page_flag);
                    // memcpy
                    memcpy(
                        bytes_of_page,
                        (void *)buffer_begin,
                        MIN(exe_load->phdr[index].p_filesz - i, page_remain)
                        );                    
                    // if (exe_load->phdr[index].p_filesz - i <  page_remain) {
                    //     for (int j =
                    //         exe_load->phdr[index].p_filesz % NORMAL_PAGE_SIZE;
                    //         j < NORMAL_PAGE_SIZE; 
                    //         ++j) 
                    //     {
                    //         bytes_of_page[j] = 0;
                    //     }
                    //     continue;
                    // } else 
                    // {
                        i += page_remain;
                        goto for_page_remain;  
                    // }                    
                }
                // if (exe_load->phdr[index].p_filesz - i < NORMAL_PAGE_SIZE) {
                //     for (int j =
                //                 exe_load->phdr[index].p_filesz % NORMAL_PAGE_SIZE;
                //             j < NORMAL_PAGE_SIZE; ++j) {
                //         bytes_of_page[j] = 0;
                //     }
                // }                
            } else 
            {
                // we can make sure it will be the data for .bss
                // long *bytes_of_page =
                    alloc_page_helper(
                                  (uintptr_t)(v_base), 
                                                pgdir,
                                             MAP_USER,
                                             page_flag);
                // uint64_t clear_begin = (uint64_t)bytes_of_page & (uint64_t)0x0fff;
                // for (int j = clear_begin;
                //         j < NORMAL_PAGE_SIZE / sizeof(long);
                //         ++j) {
                //     bytes_of_page[j] = 0;
                // }                
            }
        }
    }

    return exe_load->elf.entry + DYNAMIC_VADDR_PFFSET;    

}


extern uintptr_t load_connnetor_fix(const char * filename, uintptr_t pgdir)
{
    uint32_t elf_id = get_file_from_kernel(filename);

    char * elf_binary = (char *)elf_files[elf_id].file_content;
    uintptr_t length = *(elf_files[elf_id].file_length);   


    Elf64_Phdr *phdr = NULL;
    Elf64_Ehdr *ehdr = NULL;     

    Elf64_Phdr *ptr_ph_table = NULL;
    Elf64_Half ph_entry_count;
    int i;

    ehdr = (Elf64_Ehdr *)elf_binary;     

    // check whether `binary` is a ELF file.
    if (length < 4 || !is_elf_format((uchar *)elf_binary)) {
        return 0;  // return NULL when error!
    }

    ptr_ph_table   = (Elf64_Phdr *)(elf_binary + ehdr->e_phoff);
    ph_entry_count = ehdr->e_phnum;

    // load elf
    // debug_print_ehdr(*ehdr);
    while (ph_entry_count--) {
        phdr = (Elf64_Phdr *)ptr_ph_table;             

        if (phdr->p_type == PT_LOAD) {
            // debug_print_phdr(*phdr);
            uint64_t flag = get_page_flag(phdr);
            for (i = 0; i < phdr->p_memsz; i += NORMAL_PAGE_SIZE) {
page_remain_qemu: ;
                if (i < phdr->p_filesz) {
                    unsigned char *bytes_of_page =
                        (unsigned char *)alloc_page_helper(
                            (uintptr_t)(phdr->p_vaddr + i + DYNAMIC_VADDR_PFFSET), 
                                                    pgdir,
                                                MAP_USER,
                                                flag);
                    // not 0x1000 page align
                    uint64_t page_top = ROUND(bytes_of_page, 0x1000);

                    uint64_t page_remain = (uint64_t)page_top - \
                                    (uint64_t)(bytes_of_page);   

                    // printk("vaddr: 0x%lx, page_top: 0x%lx, page_remain: 0x%lx\n",
                    //     (uint64_t)phdr->p_vaddr + i, page_top, page_remain);                
                    
                    if (page_remain == 0)                         
                        memcpy(
                            bytes_of_page,
                            elf_binary + phdr->p_offset + i,
                            MIN(phdr->p_filesz - i, NORMAL_PAGE_SIZE));
                    else 
                    {
                        
                        memcpy(
                            bytes_of_page,
                            elf_binary + phdr->p_offset + i,
                            MIN(phdr->p_filesz - i, page_remain));
                        // if less than page remain, point has ok, and we call add 0x1000
                        if (phdr->p_filesz - i <  page_remain) {
                            for (int j =
                                    (phdr->p_vaddr + phdr->p_filesz) % NORMAL_PAGE_SIZE;
                                j < NORMAL_PAGE_SIZE; ++j) {
                                bytes_of_page[j] = 0;
                            }
                            continue;
                        } else 
                        {
                            i += page_remain;
                            goto page_remain_qemu;  
                        }
                                                                               
                    }

                    // printk("bytes_of_page: 0x%lx, page_top: 0x%lx, page_remain: 0x%lx\n",
                    //     (uint64_t)bytes_of_page, page_top, page_remain);
                    if (phdr->p_filesz - i < NORMAL_PAGE_SIZE) {
                        for (int j =
                                 (phdr->p_vaddr + phdr->p_filesz) % NORMAL_PAGE_SIZE;
                             j < NORMAL_PAGE_SIZE; ++j) {
                            bytes_of_page[j] = 0;
                        }
                    }
                } else {
                    long *bytes_of_page =
                        (long *)alloc_page_helper(
                            (uintptr_t)(phdr->p_vaddr + i + DYNAMIC_VADDR_PFFSET), 
                            pgdir,
                            MAP_USER,
                            flag);
                    uint64_t clear_begin = (uint64_t)bytes_of_page & (uint64_t)0x0fff;
                    for (int j = clear_begin;
                         j < NORMAL_PAGE_SIZE / sizeof(long);
                         ++j) {
                        bytes_of_page[j] = 0;
                    }
                }
            }
        }

        ptr_ph_table ++;
    }    

    return DYNAMIC_VADDR_PFFSET + ehdr->e_entry;
}
