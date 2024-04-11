#include <os/elf.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/kdasics.h>
#include <fs/fat32.h>
#include <utils/utils.h>
#include <assert.h>
#include <user_programs.h>

static uintptr_t load_from_binary (pcb_t * initpcb, char * binary, int * dynamic, uint64_t * memused)
{
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

    ehdr = (Elf64_Ehdr *)binary;    
    shdr = (Elf64_Shdr *)((uint64_t)binary + ehdr->e_shoff);

    Elf64_Shdr *_sec_text = NULL;
    Elf64_Shdr *_ulib_text = NULL;  
    Elf64_Shdr *_ufreelib_text = NULL;

    char * secstrs;

    // check whether `binary` is a ELF file.
    if (!is_elf_format((uchar *)binary)) {
        return 0;  // return NULL when error!
    }

    ptr_ph_table   = (Elf64_Phdr *)(binary + ehdr->e_phoff);
    ph_entry_count = ehdr->e_phnum;

    // save all useful message
    initpcb->elf.phoff = ehdr->e_phoff;
    initpcb->elf.phent = ehdr->e_phentsize;
    initpcb->elf.phnum = ehdr->e_phnum;
    initpcb->elf.entry = ehdr->e_entry;
    uintptr_t final_entry = ehdr->e_entry;
    

    // init data
	elf_bss = 0;
	elf_brk = 0;

	start_code = ~0UL;
	end_code = 0;
	start_data = 0;
	end_data = 0;

    int is_first = 1; 
    while (ph_entry_count--) {
        phdr = (Elf64_Phdr *)ptr_ph_table;
        
        if (phdr->p_type == PT_INTERP)
        {
            *dynamic = 1;
            const char * linker_path = (const char *)(phdr-> p_offset + (uintptr_t)binary);
            if (initpcb->dasics_flag ==  DASICS_STATIC)
            {
                initpcb->dasics_flag = DASICS_DYNAMIC;
                initpcb->elf.interp_load_entry = \
                        load_connector(initpcb, \
                            linker_path, \
                            DASICS_LINKER_BASE
                        );
                initpcb->elf.copy_interp_entry = \
                        load_connector(initpcb, \
                            linker_path, \
                            COPY_LINKER_BASE
                        );
                if ((int64_t)initpcb->elf.interp_load_entry <= 0 || \
                        (int64_t)initpcb->elf.copy_interp_entry <= 0)
                {
                    printk("[ERROR]: failed to Load linker %s\n", linker_path);
                    return -ENOENT;                    
                }
            } else 
            {
                initpcb->elf.interp_load_entry = \
                        load_connector(initpcb, \
                            linker_path, \
                            DYNAMIC_VADDR_PFFSET
                        );
                final_entry = initpcb->elf.interp_load_entry;                
            }
        }

        *memused += phdr->p_memsz;
   
        if (phdr->p_type == PT_LOAD) {

            map_phdr(initpcb->pgdir, phdr, binary, 0);
            
            initpcb->elf.edata =  phdr->p_vaddr + phdr->p_memsz;
            initpcb->edata = phdr->p_vaddr + phdr->p_memsz;
            if (is_first) { 
                initpcb->elf.text_begin = phdr->p_vaddr; 
                is_first = 0;
                
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

    secstrs = (char *)(shdr[ehdr->e_shstrndx].sh_offset + (uint64_t)binary);
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

    return final_entry;
}



/* prepare_page_for_kva should return a kernel virtual address */
uintptr_t load_elf(
    ElfFile *elf, pcb_t *initpcb, uint64_t *memAlloc, int *dynamic)
{
    char * elf_binary = (char *)elf->file_content;

    return load_from_binary(initpcb, elf_binary, dynamic, memAlloc);
}

/**
 * @brief load excutable file from file system
 * @param fd file descripter
 * @param pgdir page table
 * @param memAlloc mem size alloc
 * @return return the entry of the process
 */
uintptr_t fat32_load_elf(uint32_t fd, pcb_t *initpcb, uint64_t *memAlloc, int *dynamic)
{
    // read all the file into memory
    fd_t *_nfd = &current_running->pfd[get_fd_index(fd, current_running)];   
    assert(_nfd != NULL);
    char *__load_buff = (char *)LOAD_BUFFER;
    assert(_nfd->length < 24 * MB);
    fat32_lseek(fd, 0, SEEK_SET);
    uint64_t _read_length =  fat32_read(fd, __load_buff, _nfd->length);
    assert(_read_length == _nfd->length);
    fat32_close(fd);

    return load_from_binary(initpcb, __load_buff, dynamic, memAlloc);
}
