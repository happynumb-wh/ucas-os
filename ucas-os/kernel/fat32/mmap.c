#include <fs/fat32.h>
#include <os/sched.h>

#include <stdio.h>

#define MAP_SHARED 0x1
#define MAP_PRIVATE 0x2
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20

static void mmap_addr(uint64_t ptr, uint64_t size)
{
    uint64_t start = ptr;
    if (!start)
        start = pr_mm(current).mmap_base;
    uint64_t end = start + size;
    pr_mm(current).mmap_base = ROUND(end, PAGE_SIZE);
    for (uint64_t i = start; i < end; i += PAGE_SIZE)
    {
        alloc_page_helper(i, current->pgdir, MAP_USER, \
                        _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC);
        local_flush_tlb_page(i);
    }
    

}

/* 成功返回已映射区域的指针，失败返回-1*/
uint64_t fat32_mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
    // printk("[mmap] start:%x, flags: %x, len:0x%x, fd: %d, off:%d\n", start, flags, len, fd, off);
    if(len <= 0 || (uintptr_t)start % NORMAL_PAGE_SIZE != 0){
        return -EINVAL;
    }
    if(off % NORMAL_PAGE_SIZE != 0){
        off = off - off % NORMAL_PAGE_SIZE + NORMAL_PAGE_SIZE;
    }
    if(flags & MAP_ANONYMOUS){
        // return -ENOMEM;
        if(fd != -1){
            return -EBADF;
        }
        if (!start){
            // no lazy
            // start = lazy_brk(0);
            // lazy_brk(start + len);
            start = (void *)current->mm.mmap_base;
            mmap_addr((uint64_t)start, len);
            // fat32_lseek(fd, off, SEEK_SET);
            // fat32_read(fd, start, len);
        }
        if(flags & MAP_SHARED){
            // printk("map shared need to do!\n");
        }
    }else{
        //printk("file map not complete\n");
        int32_t fd_index;
        if ((fd_index = get_fd_index(fd, current_running)) == -1)
            return -EBADF;
        fd_t *nfd = &current_running->pfd[fd_index];
        int old_seek = fat32_lseek(nfd->fd_num,0,SEEK_CUR);
        // if (current_running->pfd[fd_index].length < off + len){
        //     printk("too big. len %lx, off %lx", len, off);
        //     return -EINVAL;
        // }
        if (!start){
            // start = lazy_brk(0);
            // lazy_brk(start + len);
            // start = do_brk(0);
            // if ((uint64_t)start & 0x0fff)
            //     start = (uint64_t)do_brk(ROUND((uint64_t)start, PAGE_SIZE));
            // do_brk(start + len);   
            start = (void *)current->mm.mmap_base;
            mmap_addr((uint64_t)start, len);     
        } 
        else {
            // check_addr_alloc(start, len);                   
        }
        fat32_lseek(fd, off, SEEK_SET);
        fat32_read(fd, start, len);  
        current_running->pfd[fd_index].mmap.used = 1;
        current_running->pfd[fd_index].mmap.start = start;
        current_running->pfd[fd_index].mmap.len = len;
        current_running->pfd[fd_index].mmap.prot = prot;
        current_running->pfd[fd_index].mmap.flags = flags;
        current_running->pfd[fd_index].mmap.off = off;
        fat32_lseek(nfd->fd_num,old_seek,SEEK_SET);

        // fat32_lseek(current_running->pfd[fd_index].fd_num,off+len,SEEK_SET);
        // fat32_read(fd, start, len);
    }    
    // printk("[mmap] return start: %x\n",start);
    return (uint64_t)start;
}

/* 成功返回0，失败返回-1*/
int64 fat32_munmap(void *start, size_t len)
{
    // printk("[munmap] start:0x%x, len: 0x%x\n", start, len);
    for (int i = 0; i < MAX_FILE_NUM; i++)
    {
        if (current_running->pfd[i].used && current_running->pfd[i].mmap.used && current_running->pfd[i].mmap.start == start){
            fd_t *nfd = &current_running->pfd[i];
            int old_seek = fat32_lseek(nfd->fd_num,0,SEEK_CUR);
            fat32_lseek(nfd->fd_num,nfd->mmap.off,SEEK_SET);
            // for (void *cur_page = ROUNDDOWN(start,NORMAL_PAGE_SIZE); cur_page < ROUND(start+len,NORMAL_PAGE_SIZE); cur_page += NORMAL_PAGE_SIZE){
            //     if (get_kva_of(cur_page,current_running->pgdir) != NULL) {
            //         // if it's the first page of mmap
            //         if (cur_page < ROUND(start+1,NORMAL_PAGE_SIZE)){
            //             fat32_lseek(nfd->fd_num,nfd->mmap.off,SEEK_SET);
            //             if (ROUND(start+1,NORMAL_PAGE_SIZE) < len) fat32_write(nfd->fd_num,start,(uint64_t)len);
            //             else fat32_write(nfd->fd_num,start,ROUND(start+1,NORMAL_PAGE_SIZE)-(uint64_t)start);
            //         }
            //         // if it's not the first page of mmap
            //         else {
            //             uint64_t file_off = (uint64_t)cur_page - (uint64_t)start;
            //             fat32_lseek(nfd->fd_num,nfd->mmap.off + file_off,SEEK_SET);
            //             // if it's the last page of mmap
            //             fat32_write(nfd->fd_num,cur_page,
            //                         (cur_page + NORMAL_PAGE_SIZE > (start+len))
            //                             ?(uint64_t)(start+len)%NORMAL_PAGE_SIZE:NORMAL_PAGE_SIZE);
            //         } 
            //     }
            // }
            // fat32_lseek(current_running->pfd[i].fd_num, current_running->pfd[i].mmap.off + len,SEEK_SET);
            fat32_write(current_running->pfd[i].fd_num, start, len);
            if (nfd->mmap.start == start && nfd->mmap.len == len) {
                nfd->mmap.used = 0;
                }
            else if (nfd->mmap.start == start && nfd->mmap.len > len){
                nfd->mmap.start += len;
                nfd->mmap.len -= len;
                nfd->mmap.off += len;
            }
            else printk("[munmap] the munmap in the middle / tail of the mapzone is not supported\n");
            fat32_lseek(nfd->fd_num,old_seek,SEEK_SET);
            // freepage
            for (void *cur_page = start; cur_page < start + len; cur_page += NORMAL_PAGE_SIZE){
                if (get_kva_of((uintptr_t)cur_page,current_running->pgdir) != 0){
                    // printk("freepage:%lx\n", cur_page);
                    free_page_helper((uintptr_t)cur_page, current_running->pgdir);
                }
            }
            if (current_running->edata == (uint64_t)start + len) current_running->edata = (uint64_t)start;
            if (nfd->used == 2) {
                fat32_close(nfd->fd_num);
            }
            // printk("[munmap] return\n");
            return 0;
        }
    }
    // printk("[munmap] munmap invalid\n");
    return -EINVAL;
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