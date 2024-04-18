#include <fs/fat32.h>
#include <os/sched.h>
#include <os/mmap.h>
#include <pgtable.h>
#include <stdio.h>
#include <type.h>


/* Mmap all page to zero and do copy on write */
static uintptr_t __mmap_addr(uint64_t ptr, uint64_t size, uint64_t prot)
{
    uint64_t start = ptr;
    uint64_t end;
    if (!start)
    {
        start = pr_mm(current).mmap_base;        
        end = start + size;
        pr_mm(current).mmap_base = ROUND(end, PAGE_SIZE);        
    } else 
    {
        end = start + size;        
        if (pr_mm(current).mmap_base <= end)
            pr_mm(current).mmap_base = ROUND(end, PAGE_SIZE); 
    }



    for (uint64_t i = start; i < end; i += PAGE_SIZE)
    {
        uintptr_t exits_page = get_kva_of(i, current->pgdir);
        // assert(get_kva_of(i, current->pgdir) == 0);
        if (exits_page)
        {
            memset((void *)exits_page, 0, PAGE_SIZE);
            do_mprotect((void *)i, PAGE_SIZE, prot);                            

        } else 
            alloc_page_point_phyc(i, current->pgdir, (uint64_t)__kzero_page, MAP_USER, \
                            __prot_to_page_flag(prot) & (~_PAGE_WRITE));


        // local_flush_tlb_page(i);
    }
    local_flush_tlb_all();
    return start;
}

/* mmap file to virtual address */
static uintptr_t __mmap_addr_fd(uint64_t ptr, uint64_t fd, uint64_t off, uint64_t size, uint64_t prot)
{
    assert((ptr & 0xfff) == 0);
    uint64_t start = ptr;
    uint64_t end;
    if (!start)
    {
        start = pr_mm(current).mmap_base;        
        end = start + size;
        pr_mm(current).mmap_base = ROUND(end, PAGE_SIZE);        
    } else 
    {
        end = start + size;        
        if (pr_mm(current).mmap_base < end)
            pr_mm(current).mmap_base = ROUND(end, PAGE_SIZE); 
    }

    int old_seek = fat32_lseek(fd, 0, SEEK_CUR);

    fat32_lseek(fd, off, SEEK_SET);

    for (uint64_t i = start; i < end; i += PAGE_SIZE)
    {
        // assert(get_kva_of(i, current->pgdir) == 0);
        uintptr_t exits_page = get_kva_of(i, current->pgdir);
        uint64_t max_size = end - i;
        if (exits_page)
        {
            fat32_read(fd, (void *)exits_page, PAGE_SIZE);
            do_mprotect((void *)i, PAGE_SIZE, prot);
        } else
        {

            char * buffer =  alloc_page_helper(i, current->pgdir, MAP_USER, \
                                __prot_to_page_flag(prot));  

            fat32_read(fd, buffer, min(PAGE_SIZE, max_size));
        }

        // local_flush_tlb_page(i);
    }    
    local_flush_tlb_all();


    fat32_lseek(fd, old_seek, SEEK_SET);

    return start;
}


uint64_t fat32_mmap(void *start, size_t len, uint64_t prot, uint64_t flags, uint64_t fd, off_t off)
{
    if(len <= 0 || (uintptr_t)start % NORMAL_PAGE_SIZE != 0){
        return -EINVAL;
    }
    if(off % NORMAL_PAGE_SIZE != 0){
        off = off - off % NORMAL_PAGE_SIZE + NORMAL_PAGE_SIZE;
    }
    if(flags & MAP_ANONYMOUS){
        // return -ENOMEM;
        if((int64_t)fd != -1){
            return -EBADF;
        }
        // if (!start){
        //     start = (void *)current->mm.mmap_base;
        start = (void *)__mmap_addr((uint64_t)start, len, prot);
        // }
        if(flags & MAP_SHARED){
            printk("[SHARE]: mmap share waiting to implement");
        }
    }else{
        int32_t fd_index;
        
        if (start)
        {
            if (!(flags & MAP_FIXED))
            {
                printk("[WARN]: User acquire fix add but not use MAP_FIXED");
                start = (void *)current->mm.mmap_base;
                
            }
        }

        if ((int64_t)fd == -1) 
        {
            __mmap_addr((uint64_t)start, len, prot);
            goto end;
        }

        if ((fd_index = get_fd_index(fd, current)) == -1)
            return -EBADF;
        

        start = (void *)__mmap_addr_fd((uint64_t)start, fd, off, len, prot);

    }    
end:
    return (uint64_t)start;
}

int64 fat32_munmap(void *start, size_t len)
{
    printk("[munmap] start:0x%lx, len: 0x%lx\n", start, len);
    // for (int i = 0; i < MAX_FILE_NUM; i++)
    // {
    //     if (current_running->pfd[i].used && current_running->pfd[i].mmap.used && current_running->pfd[i].mmap.start == start){
    //         fd_t *nfd = &current_running->pfd[i];
    //         int old_seek = fat32_lseek(nfd->fd_num,0,SEEK_CUR);
    //         fat32_lseek(nfd->fd_num,nfd->mmap.off,SEEK_SET);
    //         // fat32_lseek(current_running->pfd[i].fd_num, current_running->pfd[i].mmap.off + len,SEEK_SET);
    //         fat32_write(current_running->pfd[i].fd_num, start, len);
    //         if (nfd->mmap.start == start && nfd->mmap.len == len) {
    //             nfd->mmap.used = 0;
    //             }
    //         else if (nfd->mmap.start == start && nfd->mmap.len > len){
    //             nfd->mmap.start += len;
    //             nfd->mmap.len -= len;
    //             nfd->mmap.off += len;
    //         }
    //         else printk("[munmap] the munmap in the middle / tail of the mapzone is not supported\n");
    //         fat32_lseek(nfd->fd_num,old_seek,SEEK_SET);
    //         // freepage
    //         for (void *cur_page = start; cur_page < start + len; cur_page += NORMAL_PAGE_SIZE){
    //             if (get_kva_of((uintptr_t)cur_page,current_running->pgdir) != 0){
    //                 // printk("freepage:%lx\n", cur_page);
    //                 free_page_helper((uintptr_t)cur_page, current_running->pgdir);
    //             }
    //         }
    //         if (current_running->edata == (uint64_t)start + len) current_running->edata = (uint64_t)start;
    //         if (nfd->used == 2) {
    //             fat32_close(nfd->fd_num);
    //         }
    //         // printk("[munmap] return\n");
    //         return 0;
    //     }
    // }
    // printk("[munmap] munmap invalid\n");

    uintptr_t end = (uintptr_t)start + len;

    for (uintptr_t i = (uintptr_t)start; i < end; i += PAGE_SIZE)
    {
        free_page_helper(i, current->pgdir);
    }

    return 0;
    

    // return -EINVAL;
}
void *fat32_mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address){
    // printk("[mremap] old_addr = 0x%lx, old_size = %d, new_addr = 0x%lx, new_size = %d, flags = 0x%x\n",old_address,old_size,new_address,new_size,flags);
    int i;
    for (i = 0; i < MAX_FILE_NUM; i++) 
        if (current_running->pfd[i].used && current_running->pfd[i].mmap.used && current_running->pfd[i].mmap.start == old_address) break;
    if (i == MAX_FILE_NUM) return (void *)(-EINVAL);
    fd_t *nfd = &current_running->pfd[i];
    // int old_seek = fat32_lseek(nfd->fd_num,0,SEEK_CUR);
    int prot = nfd->mmap.prot;
    off_t off = nfd->mmap.off;
    fat32_munmap(old_address,old_size);
    // fat32_lseek(nfd->fd_num,off,SEEK_SET);
    fat32_mmap(new_address,new_size,prot,flags,nfd->fd_num,off);
    // fat32_lseek(nfd->fd_num,old_seek,SEEK_SET);
    return new_address;
}
int fat32_msync(void *addr, size_t length, int flags){
    if((int64_t)addr % PAGE_SIZE != 0){
        return -EINVAL;
    }
    for (int i = 0; i < MAX_FILE_NUM; i++)
    {
        if(current_running->pfd[i].used == 0){
            continue;
        }
        fd_t *nfd = &current_running->pfd[i];
        if(nfd->mmap.used == 0){
            continue;
        }
        if(nfd->mmap.start <= addr && (addr + length) <= (nfd->mmap.start + nfd->mmap.len) ){
            
            int old_seek = fat32_lseek(nfd->fd_num, 0, SEEK_CUR);
            fat32_lseek(nfd->fd_num,nfd->mmap.off + (addr - nfd->mmap.start),SEEK_SET);
            fat32_write(nfd->fd_num, addr, length);
            fat32_lseek(nfd->fd_num,old_seek,SEEK_SET);
            return 0;
        }else{
            printk("[mysync] write back too big!\n");
            assert(0);
        }
    }
    //printk("return\n");
    return -EINVAL;
}