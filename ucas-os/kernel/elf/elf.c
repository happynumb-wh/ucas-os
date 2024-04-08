#include <os/elf.h>
#include <os/sched.h>
#include <os/mm.h>
#include <fs/fat32.h>
#include <utils/utils.h>
#include <assert.h>
#include <user_programs.h>
 

// debug print message of ehdr
void debug_print_ehdr(Elf64_Ehdr ehdr)
{
    printk("=============ehdr===============\n");
    printk("ehdr.e_phoff: 0x%lx\n", ehdr.e_phoff);
    printk("ehdr.e_phnum: 0x%lx\n", ehdr.e_phnum);
    printk("ehdr.e_phentsize: 0x%lx\n", ehdr.e_phentsize);
    printk("ehdr->e_ident[0]: 0x%lx\n", ehdr.e_ident[0]);
    printk("ehdr->e_ident[1]: 0x%lx\n", ehdr.e_ident[1]);
    printk("ehdr->e_ident[2]: 0x%lx\n", ehdr.e_ident[2]);
    printk("ehdr->e_ident[3]: 0x%lx\n", ehdr.e_ident[3]);
    printk("=============end ehdr===============\n");
}

// debug print message of phdr
void debug_print_phdr(Elf64_Phdr phdr)
{
    printk("=============phdr===============\n");
    printk("phdr.p_offset: 0x%lx\n", phdr.p_offset);
    printk("phdr.p_vaddr: 0x%lx\n", phdr.p_vaddr);
    printk("phdr.p_paddr: 0x%lx\n", phdr.p_paddr);
    printk("phdr.p_filesz: 0x%lx\n", phdr.p_filesz);
    printk("phdr.p_memsz: 0x%lx\n", phdr.p_memsz);
    printk("=============end phdr===============\n");
}

/* prepare_page_for_kva should return a kernel virtual address */
uintptr_t load_elf(
    ElfFile *elf, uintptr_t pgdir, uint64_t *mem_alloc, pcb_t *initpcb ,  int *dynamic)
{
    initpcb->elf.edata = 0;

    char * elf_binary = (char *)elf->file_content;
    uintptr_t length = *(elf->file_length);

    // mm_struct of the process
    mm_struct_t *mm = &initpcb->mm;


    // data of the process memory_layout
    uint64_t elf_bss, elf_brk;
    uint64_t start_code, end_code, start_data, end_data;
    uint64_t k;


    Elf64_Phdr *phdr = NULL;
    Elf64_Shdr *shdr = NULL;
    Elf64_Ehdr *ehdr = NULL;

    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    Elf64_Phdr *ptr_ph_table = NULL;
    Elf64_Half ph_entry_count;
    int i;

    ehdr = (Elf64_Ehdr *)elf_binary;    
    shdr = (Elf64_Shdr *)((uint64_t)elf_binary + ehdr->e_shoff);

    Elf64_Shdr *_sec_text = NULL;
    Elf64_Shdr *_ulib_text = NULL;  
    Elf64_Shdr *_ufreelib_text = NULL;

    char * secstrs;

    // check whether `binary` is a ELF file.
    if (length < 4 || !is_elf_format((uchar *)elf_binary)) {
        return 0;  // return NULL when error!
    }

    ptr_ph_table   = (Elf64_Phdr *)(elf_binary + ehdr->e_phoff);
    ph_entry_count = ehdr->e_phnum;

    // save all useful message
    initpcb->elf.phoff = ehdr->e_phoff;
    initpcb->elf.phent = ehdr->e_phentsize;
    initpcb->elf.phnum = ehdr->e_phnum;
    initpcb->elf.entry = ehdr->e_entry;
    

    // init data
	elf_bss = 0;
	elf_brk = 0;

	start_code = ~0UL;
	end_code = 0;
	start_data = 0;
	end_data = 0;

    int is_first = 1; 
    // load elf
    // debug_print_ehdr(*ehdr);
    while (ph_entry_count--) {
        phdr = (Elf64_Phdr *)ptr_ph_table;

        *mem_alloc += phdr->p_memsz;

        if (phdr->p_type == PT_INTERP)
        {
            *dynamic = 1;
        }
             

        if (phdr->p_type == PT_LOAD) {
            // debug_print_phdr(*phdr);
            uint64_t flag = get_page_flag(phdr);
            for (i = 0; i < phdr->p_memsz; i += NORMAL_PAGE_SIZE) {
page_remain_qemu: ;
                if (i < phdr->p_filesz) {
                    unsigned char *bytes_of_page =
                        (unsigned char *)alloc_page_helper(
                            (uintptr_t)(phdr->p_vaddr + i), 
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
                            (uintptr_t)(phdr->p_vaddr + i), 
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
            initpcb->elf.edata =  phdr->p_vaddr + phdr->p_memsz;
            initpcb->edata = phdr->p_vaddr + phdr->p_memsz;
            if (is_first) { 
                // debug_print_phdr(*phdr);
                initpcb->elf.text_begin = phdr->p_vaddr; 
                is_first = 0;
                // Elf64_Phdr * test_phdr =  (Elf64_Phdr *)(phdr->p_offset + ehdr->e_phoff + elf_binary);
                // debug_print_phdr(*test_phdr);
            }
            k = phdr->p_vaddr;
            if (k < start_code)
                start_code = k;
            if (start_data < k)
                start_data = k;            

            k = phdr->p_vaddr + phdr->p_filesz;
            if (k > elf_bss)
                elf_bss = k;
            if ((phdr->p_flags & PF_X) && end_code < k)
                end_code = k;
            if (end_data < k)
                end_data = k;
            k = phdr->p_vaddr + phdr->p_memsz;
            if (k > elf_brk) {
                elf_brk = k;
            }                        
        }

        ptr_ph_table++;
    }

    memset(mm, 0, sizeof(mm_struct_t));

    secstrs = (char *)(shdr[ehdr->e_shstrndx].sh_offset + (uint64_t)elf_binary);
    // now, file the data depend on the section ".text" and ".ulibtext" section
    for (int j = 0; j < ehdr->e_shnum; j++)
    {
        if (shdr[j].sh_flags & SHF_ALLOC)
        {
            if (!_ulib_text && \
                    !strcmp(secstrs + shdr[j].sh_name, ".ulibtext"))
            {
                _ulib_text = &shdr[j];
                mm->ulib_sec_exist = 1;
                mm->ulibtext_start = _ulib_text->sh_addr;
                mm->ulibtext_end = _ulib_text->sh_addr + _ulib_text->sh_size;
            }
                
            if (!_sec_text && \
                    !strcmp(secstrs + shdr[j].sh_name, ".text"))
            {
                _sec_text =  &shdr[j];
                mm->text_start = _sec_text->sh_addr;
                mm->text_end   = _sec_text->sh_addr + _sec_text->sh_size;
            }  

            if (!_ufreelib_text && \
                    !strcmp(secstrs + shdr[j].sh_name, ".ufreezonetext"))
            {
                _ufreelib_text = &shdr[j];
                mm->free_sec_exit = 1;
                mm->ulibfreetext_start = _ufreelib_text->sh_addr;
                mm->ulibfreetext_end   = _ufreelib_text->sh_addr + _ufreelib_text->sh_size;
            }

        }

        if (_ulib_text && _sec_text && _ufreelib_text)
            break;
    }
    
    mm->start_code = start_code;
    mm->end_code = end_code;
    mm->start_data  = start_data;
    mm->end_data = end_data;
    mm->start_brk = initpcb->elf.edata;
    mm->start_stack = USER_STACK_ADDR;
    mm->elf_brk = elf_brk;
    mm->elf_bss = elf_bss;
    mm->mmap_base = MMAP_BASE;

    return ehdr->e_entry;
}







char * default_dll_linker = "/lib/ld-linux-riscv64-lp64d.so.1";
/**
 * @brief 从SD卡加载整个进程执行
 * @param fd 文件描述符
 * @param pgdir 页表
 * @param file_length 加载的长度
 * @param alloc_page_helper alloc_pagr_helper
 * @return return the entry of the process
 */
uintptr_t fat32_load_elf(uint32_t fd, uintptr_t pgdir, uint64_t *file_length, int *dynamic, pcb_t *initpcb)
{

    // mm_struct of the process
    mm_struct_t *mm = &initpcb->mm;

    // data of the process memory_layout
    uint64_t elf_bss, elf_brk;
    uint64_t start_code, end_code, start_data, end_data;
    uint64_t k;
    Elf64_Ehdr *_ehdr;
    Elf64_Phdr *_phdr;
    Elf64_Shdr *_shdr;

    Elf64_Shdr *_sec_text = NULL;
    Elf64_Shdr *_ulib_text = NULL;  
    Elf64_Shdr *_ufreelib_text = NULL;

    char * secstrs;
    // read all the file into memory
    fd_t *_nfd = &current_running->pfd[get_fd_index(fd, current_running)];   
    assert(_nfd != NULL);
    char *__load_buff = (char *)LOAD_BUFFER;
    assert(_nfd->length < 0x1000000);
    fat32_lseek(fd, 0, SEEK_SET);
    uint64_t _read_length =  fat32_read(fd, __load_buff, _nfd->length);
    assert(_read_length == _nfd->length);
    fat32_close(fd);

    // init elf data
    _ehdr = (Elf64_Ehdr *)__load_buff;
    _shdr = (Elf64_Shdr *)((uint64_t)__load_buff + _ehdr->e_shoff);
    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    Elf64_Off ptr_ph_table;
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;
    int i = 0;
    // check whether `binary` is a ELF file.
    if (_read_length < 4 || !is_elf_format((uchar *)_ehdr)) {
        return 0;  // return NULL when error!
    }
    ptr_ph_table   = _ehdr->e_phoff;
    ph_entry_count = _ehdr->e_phnum;
    ph_entry_size  = _ehdr->e_phentsize;

    // save all useful message
    initpcb->elf.phoff = _ehdr->e_phoff;
    initpcb->elf.phent = _ehdr->e_phentsize;
    initpcb->elf.phnum = _ehdr->e_phnum;
    initpcb->elf.entry = _ehdr->e_entry;

    int is_first = 1;
    // load the elf

    // init data
	elf_bss = 0;
	elf_brk = 0;

	start_code = ~0UL;
	end_code = 0;
	start_data = 0;
	end_data = 0;    

    while (ph_entry_count--) {
        _phdr = (Elf64_Phdr*)((uint64_t)__load_buff + ptr_ph_table);
        *file_length += _phdr->p_memsz;
        if (_phdr->p_type == PT_INTERP)
            *dynamic = 1;

        if (_phdr->p_type == PT_LOAD) {
            uint64_t page_flag = get_page_flag(_phdr);
            for (i = 0; i < _phdr->p_memsz; i += NORMAL_PAGE_SIZE) {
page_remain_k210: ;
                if (i < _phdr->p_filesz) {
                    unsigned char *bytes_of_page =
                        (unsigned char *)alloc_page_helper(
                               (uintptr_t)(_phdr->p_vaddr + i), 
                                                       pgdir,
                                                    MAP_USER,\
                                                    page_flag
                                                   );
                   
                    // not 0x1000 page align
                    uint64_t page_top = ROUND(bytes_of_page, 0x1000);

                    uint64_t page_remain = (uint64_t)page_top - \
                                    (uint64_t)(bytes_of_page);   

                    char * __ptr = (char *)((uint64_t)__load_buff + _phdr->p_offset + i);

                    if (page_remain == 0) 
                    {
                        memcpy(
                            bytes_of_page,
                            __ptr,
                            MIN(_phdr->p_filesz - i, NORMAL_PAGE_SIZE));                                           
                    } else 
                    {
                        memcpy(
                            bytes_of_page,
                            __ptr,
                            MIN(_phdr->p_filesz - i, page_remain));

                        i += page_remain;
                        goto page_remain_k210;  
                    }
                    
                } else {
                    long * __maybe_unused bytes_of_page =
                        (long *)alloc_page_helper(
                        (uintptr_t)(_phdr->p_vaddr + i), 
                                              pgdir,
                                              MAP_USER,
                                              page_flag);

                }
            }
            // save edata
            initpcb->elf.edata =  _phdr->p_vaddr + _phdr->p_memsz;
            initpcb->edata = _phdr->p_vaddr + _phdr->p_memsz;
            if (is_first) { 
                initpcb->elf.text_begin = _phdr->p_vaddr; 
                is_first = 0; 
            }
            k = _phdr->p_vaddr;
            if (k < start_code)
                start_code = k;
            if (start_data < k)
                start_data = k;            

            k = _phdr->p_vaddr + _phdr->p_filesz;
            if (k > elf_bss)
                elf_bss = k;
            if ((_phdr->p_flags & PF_X) && end_code < k)
                end_code = k;
            if (end_data < k)
                end_data = k;
            k = _phdr->p_vaddr + _phdr->p_memsz;
            if (k > elf_brk) {
                elf_brk = k;
            }            
        }
        ptr_ph_table += ph_entry_size;
    }

    memset(mm, 0, sizeof(mm_struct_t));

    secstrs = (char *)(_shdr[_ehdr->e_shstrndx].sh_offset + (uint64_t)__load_buff);
    // now, file the data depend on the section ".text" and ".ulibtext" section
    for (int j = 0; j < _ehdr->e_shnum; j++)
    {
        if (_shdr[j].sh_flags & SHF_ALLOC)
        {
            if (!_ulib_text && \
                    !strcmp(secstrs + _shdr[j].sh_name, ".ulibtext"))
            {
                _ulib_text = &_shdr[j];
                mm->ulib_sec_exist = 1;
                mm->ulibtext_start = _ulib_text->sh_addr;
                mm->ulibtext_end = _ulib_text->sh_addr + _ulib_text->sh_size;
            }
                
            if (!_sec_text && \
                    !strcmp(secstrs + _shdr[j].sh_name, ".text"))
            {
                _sec_text =  &_shdr[j];
                mm->text_start = _sec_text->sh_addr;
                mm->text_end   = _sec_text->sh_addr + _sec_text->sh_size;
            }  

            if (!_ufreelib_text && \
                    !strcmp(secstrs + _shdr[j].sh_name, ".ufreezonetext"))
            {
                _ufreelib_text = &_shdr[j];
                mm->free_sec_exit = 1;
                mm->ulibfreetext_start = _ufreelib_text->sh_addr;
                mm->ulibfreetext_end   = _ufreelib_text->sh_addr + _ufreelib_text->sh_size;
            }

        }

        if (_ulib_text && _sec_text && _ufreelib_text)
            break;
    }
    
    mm->start_code = start_code;
    mm->end_code = end_code;
    mm->start_data  = start_data;
    mm->end_data = end_data;
    mm->start_brk = initpcb->elf.edata;
    mm->start_stack = USER_STACK_ADDR;
    mm->elf_brk = elf_brk;
    mm->elf_bss = elf_bss;
    mm->mmap_base = MMAP_BASE;

    return _ehdr->e_entry;
}
