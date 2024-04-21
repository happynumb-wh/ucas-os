#include <os/elf.h>
#include <os/mm.h>
#include <assert.h>
#include <compile.h>

int map_phdr(uintptr_t pgdir, Elf64_Phdr * phdr, void * binary, uintptr_t offset)
{

    assert(phdr->p_type == PT_LOAD);

    uintptr_t page_flag = get_page_flag(phdr);

    uintptr_t map_length = 0;
    while (map_length < phdr->p_memsz) {
        if (map_length < phdr->p_filesz) {
            unsigned char *bytes_of_page =
                (unsigned char *)alloc_page_helper(
                        (uintptr_t)(phdr->p_vaddr + map_length + offset), 
                                                pgdir,
                                            MAP_USER,\
                                            page_flag
                                            );
            
            // not 0x1000 page align
            uintptr_t page_top = ROUND(bytes_of_page, PAGE_SIZE);

            uintptr_t page_base = ROUNDDOWN(bytes_of_page, PAGE_SIZE);

            uintptr_t page_remain = (uintptr_t)page_top - \
                            (uintptr_t)(bytes_of_page);   

            char * __ptr = (char *)((uintptr_t)binary + phdr->p_offset + map_length);

            if (page_remain == 0) 
            {
                uintptr_t copy_length = MIN(phdr->p_filesz - map_length, PAGE_SIZE);
                memcpy(
                    bytes_of_page,
                    __ptr,
                    copy_length);     

                // clear BSS
                if (copy_length != PAGE_SIZE)
                {
                    memset((void *)((uintptr_t)bytes_of_page + copy_length), 0, PAGE_SIZE - copy_length);
                }

                map_length += PAGE_SIZE;
            } else 
            {
                // clear low
                uintptr_t  page_low =  (uintptr_t)bytes_of_page - page_base;
                memset((void *)page_base, 0, page_low);
                

                uintptr_t copy_length = MIN(phdr->p_filesz - map_length, page_remain);
                memcpy(
                    bytes_of_page,
                    __ptr,
                    copy_length);

                // clear BSS
                if (copy_length != page_remain)
                {
                    memset((void *)((uintptr_t)bytes_of_page + copy_length), 0, PAGE_SIZE - copy_length - page_low);
                }

                map_length += copy_length;
            }
            
        } else {
            uchar * __maybe_unused bytes_of_page =
                alloc_page_helper(
                (uintptr_t)(phdr->p_vaddr + map_length + offset), 
                                        pgdir,
                                        MAP_USER,
                                        page_flag);
            
            uintptr_t page_top = ROUND(bytes_of_page, PAGE_SIZE);

            uintptr_t page_remain = (uintptr_t)page_top - \
                            (uintptr_t)(bytes_of_page);     

            assert(page_remain == 0);        
            
            // clear bss
            memset(bytes_of_page, 0, PAGE_SIZE);

            map_length += PAGE_SIZE;
        }
    }
    return 0;
}