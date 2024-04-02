#include <os/elf.h>
#include <os/sched.h>
#include <os/mm.h>
#include <fs/fat32.h>
// max elf num, only need MAX_TEST_NUM
excellent_load_t pre_elf[MAX_TEST_NUM];

// get one free elf
int get_free_elf()
{
    for (int i = 0; i < MAX_TEST_NUM; i++)
    {
        if (!pre_elf[i].used)
        {
            pre_elf[i].used = 1;
            pre_elf[i].dynamic = 0;
            init_list(&pre_elf[i].list);
            return i;
        }
            
    }
    return -1;
}

// find the elf by the name 
int find_name_elf(const char * filename)
{
    for (int i = 0; i < MAX_TEST_NUM; i++)
    {
        if (pre_elf[i].used && !strcmp(filename, pre_elf[i].name))
            return i;
    }
    return -1;
}


// init the pre_load array
void init_pre_load()
{
    for (int i = 0; i < MAX_TEST_NUM; i++)
    {
        pre_elf[i].used = 0;
        pre_elf[i].dynamic = 0;
        init_list(&pre_elf[i].list);
    }
}

// do_pre_load one file
int do_pre_load(const char * filename, int how)
{

    if (how == ELF_FREE)
    {
        int find = find_name_elf(filename);
        if (find == -1)
        {
            printk("[Error] pre_load failed to find %s\n", filename);
            return -1;
        }
        do_free_elf(&pre_elf[find]);
        return 1;
    }
    int free_index = get_free_elf();
   
    if(free_index ==  -1)
    {
        printk("[Error] failed to alloc a elf load\n");
        return -1;
    }
    strcpy(pre_elf[free_index].name, filename);

    // open the file
    int fd = fat32_openat(AT_FDCWD, filename, O_RDONLY, NULL);
    if (fd == -1)
    {
        prints("failed to open %s\n", filename);
        return -1; 
    }
    do_pre_elf_fat32(fd, &pre_elf[free_index]);
    // close the file
    fat32_close(fd);

    return 1;
}


// do pre elf
void do_pre_elf_fat32(fd_num_t fd, excellent_load_t *exe_load)
{

    fd_t *nfd = &current_running->pfd[get_fd_index(fd, current_running)];   
    assert(nfd != NULL);

    // ehdr
    Elf64_Ehdr ehdr;// = (Elf64_Ehdr *)kmalloc(sizeof(Elf64_Ehdr));
    fat32_lseek(fd, 0, SEEK_SET);
    fat32_read_uncached(fd, &ehdr, sizeof(Elf64_Ehdr));      

    Elf64_Phdr phdr;  

    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    Elf64_Off ptr_ph_table;
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;
    int i = 0;
    // check whether `binary` is a ELF file.
    if (nfd->length < 4 || !is_elf_format(&ehdr)) {
        return 0;  // return NULL when error!
    }
    ptr_ph_table   = ehdr.e_phoff;
    ph_entry_count = ehdr.e_phnum;
    ph_entry_size  = ehdr.e_phentsize;
    // save all useful message
    exe_load->elf.phoff = ehdr.e_phoff;
    exe_load->elf.phent = ehdr.e_phentsize;
    exe_load->elf.phnum = ehdr.e_phnum;
    exe_load->elf.entry = ehdr.e_entry;
    exe_load->phdr_num = 0;
    int index = 0;
    int is_first = 1;
    // load the elf
    while (ph_entry_count--) {
        fat32_lseek(fd, ptr_ph_table, SEEK_SET);
        fat32_read_uncached(fd, &phdr, sizeof(Elf64_Phdr));        
        if (phdr.p_type == PT_INTERP)
        {
            exe_load->dynamic = 1;
        }    

        if (phdr.p_type == PT_LOAD) {
            // copy phdr
            memcpy(&exe_load->phdr[index++], &phdr, sizeof(Elf64_Phdr));
            uint64_t length = phdr.p_filesz;
            uint64_t al_length = 0;
            uint64_t fd_base = phdr.p_offset;
            // the ptr
            fat32_lseek(fd, phdr.p_offset, SEEK_SET);
            while (length)
            {
                uint64_t offset_remain = ROUND(fd_base, 0x1000) - fd_base;

                char * buffer = kmalloc(PAGE_SIZE);
                page_node_t* alloc =  &pageRecyc[GET_MEM_NODE((uintptr_t)buffer)];
                // add list
                list_del(&alloc->list);
                list_add_tail(&alloc->list, &exe_load->list);
                uint32_t read_length;
                if (offset_remain)
                    read_length = MIN(offset_remain, phdr.p_filesz - al_length);
                else 
                    read_length = ((phdr.p_filesz - al_length) >= PAGE_SIZE) ? PAGE_SIZE :
                                            phdr.p_filesz - al_length;
                
                fat32_read_uncached(fd, buffer, read_length);
                fd_base += read_length;
                al_length += read_length;
                length -= read_length;
            }
            exe_load->elf.edata = phdr.p_vaddr + phdr.p_memsz;
            exe_load->phdr_num++;
            // 
            if (is_first) { 
                exe_load->elf.text_begin = phdr.p_vaddr; 
                is_first = 0; 
            }
        }
        ptr_ph_table += ph_entry_size;
    }   
}


// do free the elf
void do_free_elf(excellent_load_t *exe_load)
{
    exe_load->used = 0;
    exe_load->dynamic = 0;
    page_node_t * recycle_entry = NULL, *recycle_q;
    list_for_each_entry_safe(recycle_entry, recycle_q, &exe_load->list, list)
    {
        kfree_voilence(recycle_entry->kva_begin);
    }
}

// fast load elf
uintptr_t fast_load_elf(const char * filename, uintptr_t pgdir, uint64_t *file_length, pcb_t *initpcb)
{
    initpcb->elf.edata = 0;
    int elf_id;
    if ((elf_id = find_name_elf(filename)) == -1)
    {
        printk("[Error] fast_load_elf failed to find %s\n", filename);
        return 0;
    }
    // the exe_load
    excellent_load_t * exe_load = &pre_elf[elf_id];
    uint64_t buffer_begin;
    list_node_t * load_node = exe_load->list.next;
    for (int index = 0; index < exe_load->phdr_num; index++)
    {
        uint64_t page_flag = get_page_flag(&exe_load->phdr[index]);
        for (int i = 0; i < exe_load->phdr[index].p_memsz; i += NORMAL_PAGE_SIZE)
        {
for_page_remain: ;
            uintptr_t v_base = exe_load->phdr[index].p_vaddr + i;
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
                    if (!(exe_load->phdr[index].p_flags & PF_W) /*&& !exe_load->dynamic*/)
                    {
                        alloc_page_point_phyc(v_base, \ 
                                              pgdir, \
                                              buffer_begin, \
                                              MAP_USER, \
                                              page_flag
                                            );
                        bytes_of_page = (uchar *)buffer_begin;
                    } else 
                    {
                        // this will never happen for the first phdr
                        // else alloc and copy
                        bytes_of_page = alloc_page_helper(v_base, pgdir, MAP_USER, page_flag);
                        memcpy(
                            bytes_of_page,
                            buffer_begin,
                            MIN(exe_load->phdr[index].p_filesz - i, NORMAL_PAGE_SIZE));
                    }
                } else 
                {
                    // if not align, we can make sure it will be the data phdr
                    bytes_of_page = alloc_page_helper(v_base, pgdir, MAP_USER, page_flag);
                    // memcpy
                    memcpy(
                        bytes_of_page,
                        buffer_begin,
                        MIN(exe_load->phdr[index].p_filesz - i, page_remain)
                        );     
                    // we need a new method to fill in the blank    
                    if (exe_load->phdr[index].p_filesz - i <  page_remain) {
                        for (int j =
                            // exe_load->phdr[index].p_filesz % NORMAL_PAGE_SIZE;
                            ((int)exe_load->phdr[index].p_vaddr +  exe_load->phdr[index].p_filesz) % NORMAL_PAGE_SIZE;
                            j < NORMAL_PAGE_SIZE; 
                            ++j) 
                        {
                            bytes_of_page[j] = 0;
                        }
                        continue;
                    } else 
                    {
                        i += page_remain;
                        goto for_page_remain;  
                    }                    
                }
                if (exe_load->phdr[index].p_filesz - i < NORMAL_PAGE_SIZE) {
                    for (int j =
                                // exe_load->phdr[index].p_filesz % NORMAL_PAGE_SIZE;
                            ((int)exe_load->phdr[index].p_vaddr + exe_load->phdr[index].p_filesz) % NORMAL_PAGE_SIZE;    
                            j < NORMAL_PAGE_SIZE; ++j) {
                        bytes_of_page[j] = 0;
                    }
                }                
            } else 
            {
                // we can make sure it will be the data for .bss
                long *bytes_of_page =
                    (long *)alloc_page_helper(
                    (uintptr_t)(exe_load->phdr[index].p_vaddr + i), 
                                            pgdir,
                                            MAP_USER,
                                            page_flag);
                uint64_t clear_begin = (uint64_t)bytes_of_page & (uint64_t)0x0fff;
                for (int j = clear_begin;
                        j < NORMAL_PAGE_SIZE / sizeof(long);
                        ++j) {
                    bytes_of_page[j] = 0;
                }                
            }
        }
    }
    // copy the elf and edata
    memcpy(&initpcb->elf, &exe_load->elf, sizeof(ELF_info_t));
    initpcb->edata = exe_load->elf.edata;

    return initpcb->elf.entry;

}


/**
 * @brief the execve load for fast load and just recycle part of space
 * 
 */
uintptr_t fast_load_elf_execve(const char * filename, pcb_t *initpcb)
{
    // change to pcb_t




}