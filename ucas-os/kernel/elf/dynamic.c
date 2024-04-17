#include <os/elf.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/string.h>
#include <fs/fat32.h>
#include <compile.h>
#include <stdio.h>


int linker_in_buffer = 0;

uintptr_t load_connector(pcb_t *initpcb, const char * path, uintptr_t offset)
{
    printk("connector: %s\n", path);

    uint64_t _read_length;
    // Pointer
    Elf64_Ehdr *_ehdr;
    Elf64_Phdr *_phdr;
    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    Elf64_Phdr * ptr_ph_table;
    Elf64_Half ph_entry_count;

    // load linker
    if (strcmp("/lib/ld-linux-riscv64-lp64d.so.1", path) && \
            strcmp("/lib/ld-linux-riscv64-lp64.so.1", path))
    {
        printk("[ERROR]: Usupport %s\n", path);
        return -ENOENT;        

    }

    if (!linker_in_buffer)
    {
        int fd;
        if((fd = fat32_openat(AT_FDCWD, path, O_RDONLY, 0)) <= 0) {
            printk("[ERROR]: failed to open %s\n", path);
            return -ENOENT;
        }
        fd_t *_nfd = &current_running->pfd[get_fd_index(fd, current_running)];   
        fat32_lseek(fd, 0, SEEK_SET);
        _read_length =  fat32_read(fd, (char *)__linker_buffer, _nfd->length);
        assert(_read_length == _nfd->length);
        fat32_close(fd);
        linker_in_buffer = 1;
    }

   

    // init elf data
    _ehdr = (Elf64_Ehdr *)__linker_buffer;

    // check whether `binary` is a ELF file.
    if (!is_elf_format((uchar *)_ehdr)) {
        return 0;  // return NULL when error!
    }
    ptr_ph_table   = (Elf64_Phdr *)(__linker_buffer + _ehdr->e_phoff);
    ph_entry_count = _ehdr->e_phnum;

    while (ph_entry_count--) {
        _phdr = ptr_ph_table;

        if (_phdr->p_type == PT_LOAD) {
            map_phdr(initpcb->pgdir, _phdr, __linker_buffer, offset);
        }
        ptr_ph_table ++;
    }    

    return offset + _ehdr->e_entry;

}

