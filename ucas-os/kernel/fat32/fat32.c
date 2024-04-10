#include <os/socket.h>
#include <os/errno.h>
#include <os/sched.h>
#include <fs/fat32.h>
#include <utils/utils.h>
#include <common.h>
#include <assert.h>
#include <screen.h>
#include <stdio.h>

// cwd
char cwd_path[FAT32_MAX_PATH] = {0};
fat_t fat;
inode_t cwd, root;

/**
 * @brief init fat32 and print important message
 */
char* passwd = "stu:x:1000:1000:stu:/home/stu:/bin/bash\n";
int fat32_init()
{
    printk("> [FAT32 init] begin!\n");
    init_pipe();
    uint8_t *b =  kmalloc(PAGE_SIZE);
    //the DBR of SD card
    uint8_t result = sd_read_sector(b, FAT_FIRST_SEC, 1);
    if (result != 0x00) 
        printk("disk read error %x", result);
    /* we don't judge the file system by the BsFilSysType */
    if(memcmp((char*)(b + BS_FileSysType), "FAT32", 5)){
        printk("not FAT32 volume, init fat32 failed\n");
        while(1);
    }
    printk("> [FAT32 init] read zero sector succeeded!\n");
    //读取sd信息
    fat.bpb.byts_per_sec    = *(uint8 *)(b + BPB_BytsPerSec) + (*(uint8 *)(b + BPB_BytsPerSec + 1))*256;
    fat.bpb.sec_per_clus    = *(b + BPB_SecPerClus);
    fat.bpb.rsvd_sec_cnt    = *(uint16 *)(b + BPB_RsvdSexCnt);
    fat.bpb.fat_cnt         = *(b + BPB_NumFATs);
    fat.bpb.hidd_sec        = *(uint32 *)(b + BPB_HiddSec);
    fat.bpb.tot_sec         = *(uint32 *)(b + BPB_TocSec32);
    fat.bpb.fat_sz          = *(uint32 *)(b + BPB_FATSz32);
    fat.bpb.root_clus       = *(uint32 *)(b + BPB_RootClus);//根目录簇号
    // clus 2's sector
    fat.first_data_sec = fat.bpb.rsvd_sec_cnt + fat.bpb.hidd_sec + fat.bpb.fat_cnt * fat.bpb.fat_sz;
    fat.data_sec_cnt = fat.bpb.tot_sec - fat.first_data_sec;
    fat.data_clus_cnt = fat.data_sec_cnt / fat.bpb.sec_per_clus;
    fat.byts_per_clus = fat.bpb.sec_per_clus * fat.bpb.byts_per_sec;
    fat.next_free = search_empty_cluster();
    /* init lock */
    // initsleeplock(&fat.fat_lock);
    printk("> [FAT32 init] byts_per_sec: %d\n", fat.bpb.byts_per_sec);
    printk("> [FAT32 init] root_clus: %d\n", fat.bpb.root_clus);
    printk("> [FAT32 init] sec_per_clus: %d\n", fat.bpb.sec_per_clus);
    printk("> [FAT32 init] fat_cnt: %d\n", fat.bpb.fat_cnt);
    printk("> [FAT32 init] fat_sz: %d\n", fat.bpb.fat_sz);
    printk("> [FAT32 init] fat_rsvd: %d\n", fat.bpb.rsvd_sec_cnt);
    printk("> [FAT32 init] fat_hid: %d\n", fat.bpb.hidd_sec);
    printk("> [FAT32 init] fat_first_data_sec: %d\n", fat.first_data_sec);
    printk("> [FAT32 init] first_data_sec: %d\n", fat.first_data_sec);
    printk("> [FAT32 init] data_clus_cnt: %d\n", fat.data_clus_cnt);
    printk("> [FAT32 init] byts_per_clus: %d\n", fat.byts_per_clus);
    printk("> [FAT32 init] next free: 0x%x\n", fat.next_free);
    memset(cwd.name, 0, 256);
    memset(root.name, 0, 256);
    strcat(cwd_path,"/");
    strcat(root.name,"/");
    root.clus_num = fat.bpb.root_clus;
    root.first_clus = fat.bpb.root_clus;
    root.sector_num = first_sec_of_clus(root.clus_num);
    cwd.clus_num = root.clus_num;
    cwd.sector_num = root.sector_num;
    cwd.first_clus = fat.bpb.root_clus;
    kfree(b);

    return 0;
}

/**
 * @brief 打开文件，如果path为绝对路径，则不关心fd的内容
 * 如果是相对路径，如果fd等于AT_FDCW则以当前工作目录为基址
 * 如果fd不等于AT_FDCW则需要以fd目录为基址
 * @param fd 文件描述符
 * @param path 工作路径
 * @param flags 标记，总共有:
 *  O_RDONLY 0x000
 *  O_WRONLY 0x001
 *  O_RDWR 0x002
 *  O_CREATE 0x40
 *  O_DIRECTORY 0x0200000
 * @param mode 似乎要暂时忽略
 * @return 成功返回得到的文件描述符，失败返回-1
 */
int fat32_openat(fd_num_t fd, const char *path, uint32 flags, uint32 mode)
{
    // printk("[openat] fd:%d, path: %s\n", fd, path);
    // printk("[openat] try to open path %s, creat: %d\n",path,(flags & O_CREATE));
    uint32_t absolute_path = is_ab_path(path);
    /* find the pre clus */
    uint32_t base_clus = 0;
    if (absolute_path) 
        base_clus = root.first_clus;
    else if (fd == AT_FDCWD) 
        base_clus = cwd.first_clus;
    else if (fd != AT_FDCWD) {
        int fd_index = get_fd_index(fd, current_running);
        if (fd_index == -1) return -EBADF;
        base_clus = current_running->pfd[fd_index].first_clus_num;
    }
    // pre_path(path, base_clus);
    fd_num_t new = get_one_fd(current_running);
    if (new == -1) return -EMFILE;
    fd_t *new_fd = &current_running->pfd[get_fd_index(new, current_running)];
    
    // handle "/dev/null" "/dev/null/invalid" "/dev/zero"
    // "/etc/passwd" "/etc/localtime"  "/proc/mounts" "/proc/meminfo" 
    // "/ls"
    if (open_special_path(new_fd, path))
    {
        // init_share_fd(new_fd);
        // printk("[openat] special path!\n");
        return new;
    }
    type_t default_type = ((flags & O_DIRECTORY) == 0) ? FILE_FILE : FILE_DIR;
    char copy_path[strlen(path) + 2];
    /* like / */
    /* open a dir */
    strcpy(copy_path, pre_path(path, base_clus));
    /* clear '/' */
    char *dname = copy_path;
    uint32_t old_dir_clus = base_clus;
    uint32_t __maybe_unused new_dir_clus = base_clus;    
    dentry_t *find_result = NULL;
    char * buff = kmalloc(PAGE_SIZE);
    dir_pos_t pos;
    if (copy_path[0] == '\0') {
        strcpy(new_fd->name, path);
        new_fd->flags = flags | S_IFDIR;
        new_fd->nlink = 1;
        new_fd->pos = 0;
        new_fd->length = 0;
        if (absolute_path) {
            new_fd->first_clus_num = root.first_clus;
            new_fd->cur_clus_num = root.clus_num;
            new_fd->cur_sec = root.sector_num;
        }
        else {
            new_fd->first_clus_num = cwd.first_clus;
            new_fd->cur_clus_num = cwd.clus_num;
            new_fd->cur_sec = cwd.sector_num;  
        }
        kfree(buff);
        // init_share_fd(new_fd);
        // printk("pid:%d, fd:%d, share fd:%x\n", current_running->pid, new, new_fd->share_fd);
        return new;
    }
    while (1) {
        if (!get_next_name(dname)) {
            // printk("openat: dname = %s, old_dir_clus = %d\n",dname,old_dir_clus);
            if ((flags & O_DIRECTORY) == 0)
                find_result = search3(dname, old_dir_clus, buff, &pos);
            else 
                find_result = search(dname, old_dir_clus, buff, default_type, &pos);
            // assert(find_result != NULL);
            /* not found */
            if (!find_result) {
                if (flags & O_CREATE) {
                    // 创建一个文件或者文件夹，类型为default_type
                    // 1. _new_crate
                    find_result = create_new(dname, old_dir_clus, buff, default_type, &pos);
                    // 2. set_fd_from_dentry
                    // printk("pos: sec %d, offset: %d", pos.sec, pos.offset);
                    // while(1);
                    set_fd_from_dentry(current_running, dname, get_fd_index(new, current_running), find_result, flags, &pos);
                    // init_share_fd(new_fd);
                    // printk("[openat] fd:%d, fd_name = %s\n", new, new_fd->name);
                    kfree(buff);
                    return new;
                }
                fat32_close(new);
                kfree(buff);
                // printk("openat: %s is not found\n",dname);
                return -ENOENT;
            }
            // debug_print_dt(find_result);
            set_fd_from_dentry(current_running, dname, get_fd_index(new, current_running), find_result, flags, &pos);
            break;
        }
        
        find_result = search(dname, old_dir_clus, buff, FILE_DIR, &pos);
        if (flags & O_CREATE && find_result == NULL){
            find_result = create_new(dname, old_dir_clus, buff, FILE_DIR, NULL);
        } else if (find_result == NULL)
            // not found
            return -ENOENT;
        // debug_print_dt(find_result);
        old_dir_clus = get_cluster_from_dentry(find_result);
        
        dname += (strlen(dname) + 1);
    }
    kfree(buff);
    // }
    // debug_print_dt(new_fd);
    // init_share_fd(new_fd);
    // printk("[openat] pid:%d, fd:%d, share fd:%x\n", current_running->pid, new, new_fd->share_fd);
    // printk("return %d\n", new);
    return new;
}


/**
 * @brief 关闭一个文件描述符
 * @param fd 想要关闭的文件描述符
 */
int64 fat32_close(fd_num_t fd)
{
    int fd_index = get_fd_index_without_redirect(fd, current_running);
    if (fd_index < 0) return -EBADF;
    else {
        // recycle_one_fd(fd);
        // printk("[close] closing fd %d, index = %d\n",fd,fd_index);
        fd_t *nfd = &current_running->pfd[fd_index];
        if (nfd->mmap.used) {
            nfd->used = 2; // ZOMBIE FOR MMAP
            return 0;
        }
        for (int i=0;i<MAX_FILE_NUM;i++)
            if (current_running->pfd[i].used && current_running->pfd[i].redirected && current_running->pfd[i].redirected_fd_index == fd_index) {
                    int backup_fd_num = current_running->pfd[i].fd_num;
                    copy_fd(&current_running->pfd[i],&current_running->pfd[fd_index]);
                    current_running->pfd[i].fd_num = backup_fd_num;
                }
        if (nfd->dev == DEV_PIPE){
            if (nfd->is_pipe_read) close_pipe_read(nfd->pip_num);
            else if (nfd->is_pipe_write) close_pipe_write(nfd->pip_num);
        }
        else if (nfd->dev == DEV_SOCK) close_socket(nfd->sock_num);
        nfd->pos = 0;
        nfd->length = 0;
        nfd->used = FD_UNUSED;
        nfd->dev = DEV_DEFAULT;
        nfd->flags = 0;
        nfd->mode = 0;
        nfd->redirected = 0;
        nfd->mmap.used = 0;
        // if(nfd->share_fd != 0 && --nfd->share_fd->share_num <= 0){
        //     kfree(nfd->share_fd);
        // }
        // nfd->share_fd = 0;
        return 0;
    }

}

/**
 * @brief fat32读系统调用，从文件当中读取count个字符 
 * @param fd 需要读的文件描述符
 * @param buf 来自用户态的缓冲区
 * @param count 读取的数目
 */
int64 fat32_read(fd_num_t fd, char *buf, size_t count)
{
    // for (uint64_t i = (uint64_t)buf; i < (uint64_t)buf + count; i+=PAGE_SIZE)
    //     handle_page_fault_store(NULL, i, NULL);
    
    // printk("[read] fd:%d,buf:%lx, count: %d(0x%x)\n", fd, buf,count, count);
    // get nfd now
    int fd_index = get_fd_index(fd, current_running);
    if (fd_index == -1) return -EBADF;
    fd_t *nfd = &current_running->pfd[fd_index];
    //update fd
    // if(nfd->share_fd != 0 && nfd->share_fd->version > nfd->version){
    //     memcpy(nfd, nfd->share_fd, sizeof(fd_t));
    // }   
    // printk("[read] try to read fd = %d(index = %d), buf = 0x%x, count = %d\n",fd,fd_index,buf,count);
    assert(nfd != NULL);

    // handle "/etc/passwd", "/etc/localtime", DEV_STDIN, DEV_STDOUT
    // DEV_STDERR, DEV_NULL, DEV_ZERO
    int special_return = read_special_path(nfd, buf, count);
    // make sure not return -1 for pip_read
    if (special_return != -1)
        return special_return;
    
    // permission deny
    if (nfd->flags & O_DIRECTORY || nfd->flags & O_WRONLY) 
        return -EINVAL;
    char * buffer = kmalloc(PAGE_SIZE);
    char * buf_point = buf;
    // debug_print_fd(nfd);
    if (nfd->length < nfd->pos) {
        kfree(buffer);
        return -EFAULT;
    }
    uint32_t real_count = min(count, nfd->length - nfd->pos);
    // printk("[read] nfd->length = %d, nfd->pos = %d\n",nfd->length,nfd->pos);
    uint32_t ncount = 0;
    /* record */
    uint32_t old_clus_num = nfd->pos / CLUSTER_SIZE;
    uint32_t old_sec_num = nfd->pos / SECTOR_SIZE;
    uint32_t new_clus_num = old_clus_num;
    uint32_t new_sec_num = old_sec_num;
    while (ncount < real_count) {
        // change in 2022 7.16
        // size_t pos_offset_in_buf = nfd->pos % BUFSIZE;
        size_t pos_offset_in_buf = nfd->pos % SECTOR_SIZE;
        // printk("pos_offset_in_buf: %d\n", pos_offset_in_buf);
        size_t readsize = min(BUFSIZE - pos_offset_in_buf, real_count - ncount);
        /* read readsize */
        // printk("fat32_read: 0x%lx,%d,%d\n",buffer, nfd->cur_sec, READ_MAX_SEC);
        disk_read(buffer, nfd->cur_sec, READ_MAX_SEC);
        /* memcpy */
        memcpy(buf_point, buffer + pos_offset_in_buf, readsize); 
        buf_point += readsize;
        ncount += readsize;
        nfd->pos += readsize;
        new_clus_num = nfd->pos / CLUSTER_SIZE;
        new_sec_num = nfd->pos / SECTOR_SIZE;
        if (new_clus_num - old_clus_num) {
            nfd->cur_clus_num = next_cluster(nfd->cur_clus_num);
            nfd->cur_sec = ((first_sec_of_clus(nfd->cur_clus_num) + \
                            (nfd->pos % CLUSTER_SIZE) / SECTOR_SIZE));
        } else 
        {
            if (new_sec_num - old_sec_num) 
                nfd->cur_sec += (new_sec_num - old_sec_num);
        }
        old_clus_num = new_clus_num;
        old_sec_num = new_sec_num;
    }
    kfree(buffer);
    // printk("[read] read; fd = %d(%d), buf = 0x%lx, real_count = %d\n",fd,fd_index,buf,real_count);
    // nfd->version++;
    // memcpy(nfd->share_fd, nfd, sizeof(fd_t));
    return real_count;
}

int64 fat32_write(fd_num_t fd, const char *buf, uint64_t count)
{
    /* tmp */
    // printk("[write] try to write; fd = %d, buf = 0x%x(%s), count = %d\n",fd,buf,buf,count);
    // if(count == 4096){
    //     printk("count\n");
    // }
    // get nfd now
    int fd_index = get_fd_index(fd, current_running);
    // printk("fd_index: %d\n", fd_index);
    if (fd_index == -1){
        // TODO: write error to let ferror read
        return -EBADF;
    }
    fd_t *nfd = &current_running->pfd[fd_index];
    assert(nfd != NULL);
    // if(nfd->share_fd != 0 && nfd->share_fd->version > nfd->version){
    //     memcpy(nfd, nfd->share_fd, sizeof(fd_t));
    // }  
    // handle "/etc/passwd", "/etc/localtime", DEV_STDIN, DEV_STDOUT
    // DEV_STDERR, DEV_NULL, DEV_ZERO
    int special_return = write_special_path(nfd, buf, count);
    // make sure pipe write not return -1
    if (special_return != -1)
        return special_return;
    // permission deny
    if ((nfd->flags & O_DIRECTORY) || (nfd->flags & O_RDONLY)) 
        return -EINVAL;   
    if (count == 0)
        return 0;
    char * buffer = kmalloc(PAGE_SIZE);
    char *buf_point = (char *)buf;
    // debug_print_fd(nfd);
    uint32_t ncount = 0;
    uint32_t final_length = nfd->pos + count;
    /* record */
    uint32_t old_clus_num = nfd->pos / CLUSTER_SIZE;
    uint32_t old_sec_num = nfd->pos / SECTOR_SIZE;
    uint32_t new_clus_num = old_clus_num;
    uint32_t new_sec_num = old_sec_num;
    int first_flag = 0;
    /* for the windows, null files*/    
    if (nfd->first_clus_num == 0) {
        first_flag = 1;
        nfd->first_clus_num = search_empty_cluster();
        nfd->cur_clus_num = nfd->first_clus_num;
        nfd->cur_sec = first_sec_of_clus(nfd->first_clus_num);
    }
    // debug_print_fd(nfd);
    while (ncount < count) {
        // change in 2022 7.16
        // size_t pos_offset_in_buf = nfd->pos % BUFSIZE;
        size_t pos_offset_in_buf = nfd->pos % SECTOR_SIZE;

        // write as many as possiable
        size_t writesize = min(BUFSIZE - pos_offset_in_buf, count - ncount);
        disk_read(buffer, nfd->cur_sec, READ_MAX_SEC);
        /* write */
        memcpy(buffer + pos_offset_in_buf, buf_point, writesize);
        /* consider the multicore, we must write after malloc */
        /* judge if need malloc */
        buf_point += writesize;
        ncount += writesize;
        nfd->pos += writesize;
        new_clus_num = nfd->pos / CLUSTER_SIZE;
        new_sec_num = nfd->pos / SECTOR_SIZE; 
        uint32_t alloc_clus;
        if (new_clus_num - old_clus_num) {
            /* TODO need alloc one new cluster */
            if (next_cluster(nfd->cur_clus_num) == M_FAT32_EOC) {
                alloc_clus = search_empty_cluster();
                /* write back fat table */
                write_fat_table(nfd->cur_clus_num, alloc_clus);
            }
            disk_write(buffer, nfd->cur_sec, READ_MAX_SEC);
            nfd->cur_clus_num = next_cluster(nfd->cur_clus_num);
            nfd->cur_sec = ((first_sec_of_clus(nfd->cur_clus_num) + \
                            (nfd->pos % CLUSTER_SIZE) / SECTOR_SIZE));
        } else 
        {
            disk_write(buffer, nfd->cur_sec, READ_MAX_SEC); 
            if (new_sec_num - old_sec_num) {
                nfd->cur_sec += (new_sec_num - old_sec_num);       
            }  
        }
        old_clus_num = new_clus_num;
        old_sec_num = new_sec_num;
    }
    /* update length */
    if (count && final_length > nfd->length) {
        disk_read(buffer, nfd->dir_pos.sec, 1);
        dentry_t * dentry = (dentry_t *)(buffer + nfd->dir_pos.offset);
        dentry->file_size = final_length;
        if (first_flag) {
            set_cluster_for_dentry(dentry, nfd->first_clus_num);
        }
        disk_write(buffer, nfd->dir_pos.sec, 1);
        nfd->length = dentry->file_size;
        update_fd_length(nfd);
        // debug_print_dt(dentry);
        // debug_print_fd(nfd);
    }
    kfree(buffer);
    // printk("[write] write; fd = %d(%d), buf = 0x%lx, count = %d; final file length = %d\n",fd,fd_index,buf,count,nfd->length);
    // nfd->version++;
    assert(nfd != 0);   
    // assert(nfd->share_fd != 0);
    // memcpy(nfd->share_fd, nfd, sizeof(fd_t));
    return count;
}


/**
 * @brief lseek 初赛无要求，但是加载SD卡时需要用到，暂时不支持超出文件大小的偏移，
 * 这需要新分配簇，稍微有点麻烦
 * @param fd 文件描述符
 * @param off 偏移量，允许为负数
 * @param whence 基准
 * SEEK_SET：相对于文件头
 * SEEK_CUR：相对于文件中
 * SEEK_END：相对于文件末尾
 * @return 
 */
int64 fat32_lseek(fd_num_t fd, size_t off, uint32_t whence)
{
    // get nfd now
    int fd_index = get_fd_index(fd, current_running);
    if (fd_index < 0) return -EBADF;
    fd_t *nfd = &current_running->pfd[fd_index];
    if (nfd->dev != DEV_DEFAULT) return -EINVAL;

    uint64_t old_off = nfd->pos;
    // if(nfd->share_fd != 0 && nfd->share_fd->version > nfd->version){
    //     memcpy(nfd, nfd->share_fd, sizeof(fd_t));
    // }   
    // printk("[lseek] try to lseek fd %d(%d) to %ld, whence = %d\n",fd,fd_index,off,whence);
    switch(whence){
        case SEEK_SET:
            nfd->pos = off;
            break;
        case SEEK_CUR:
            nfd->pos += off;
            break;
        case SEEK_END:
            nfd->pos = nfd->length + off;
            break;
        default: assert(0);
            break;
    }

    if (old_off == nfd->pos) return nfd->pos;

    // printk("[lseek] entering update_fd_from_pos\n");
    if (nfd->pos < nfd->length) update_fd_from_pos(nfd, nfd->pos);
    // printk("[lseek] leaving update_fd_from_pos\n");
    // nfd->version++;
    assert(nfd != 0);
    // assert(nfd->share_fd != 0);
    // memcpy(nfd->share_fd, nfd, sizeof(fd_t));
    // printk("[lseek] return: %d\n", nfd->pos);
    return nfd->pos;
}


long fat32_getcwd(char *buf, size_t size){
    if(size<2){
        return 0;
    }else{
        memcpy(buf, cwd_path, strlen(cwd_path)+1);
    }
    return (long)buf;    
}

int fat32_chdir(char *dest_path)
{
    char path[FAT32_MAX_PATH];
    char temp[FAT32_MAX_PATH];
    int len = strlen(dest_path);
    strcpy(temp, dest_path);
    int count = 0;
    for (int i = 0; i < len; i++)
    {
        if(temp[i] == '.' && temp[i + 1] =='/'){
            i++;
        }else{
            path[count] = temp[i];
            count++;
        }    
    }
    path[count] = '\0';
    len = count;
    // printk("path: %s, len: %d, %d\n", path, len, strlen(path));
    // while(1);
    if(!strcmp("/", path))
    {
        memset(cwd_path, 0, FAT32_MAX_PATH);
        cwd_path[0] = '/';
        cwd_path[1] = '\0';
        cwd.first_clus = root.first_clus;
        return 0;
    }
    
    
    inode_t f_dir;
    if(path[0] == '/')
    {
        f_dir = find_dir(root, path + 1);
    }
    else
    {
        f_dir = find_dir(cwd, path);
    }
    if(f_dir.name[0] == 0xff)
    {
        printk("No such directory.\n\r");
        return -ENOENT;
    }
    char *buff = kmalloc(PAGE_SIZE);
    dentry_t* dentry = search(f_dir.name, f_dir.first_clus, buff, FILE_DIR, NULL);
    if(dentry == NULL){
        kfree(buff);
        return -ENOENT;
    }else{
        cwd.first_clus = get_cluster_from_dentry(dentry);
        cwd.sector_num = first_sec_of_clus(cwd.first_clus);
        strcpy(cwd.name, f_dir.name);
        if(dest_path[0] == '/'){
            memset(cwd_path, 0, FAT32_MAX_PATH);
            memcpy(cwd_path, dest_path, strlen(dest_path));
        }else{
            strcat(cwd_path, dest_path);
        }
        kfree(buff);
        // printk("cwd first clus: %d, cwd name: %s, cwd.path:%s",cwd.first_clus, cwd.name, cwd_path);
        return 0;
    }
}

int fat32_mkdirat(fd_num_t dirfd, const char *path_const, uint32_t mode)
{  
    
    char *buf = kmalloc(PAGE_SIZE);
    int len = strlen(path_const);
    char path[len + 1];
    strcpy(path, path_const);
    dentry_t __maybe_unused *entry = (dentry_t *)buf;
    inode_t dir;
    if(path[0] == '/' && path[1] == '\0'){
        kfree(buf);
        return 0;
    }
    if(path[len - 1] == '/'){
        path[len - 1] = '\0';
    }
    if(path[0] == '/'){
        dir = find_dir(root, path);
    }else{
        if(dirfd == AT_FDCWD)
        {
            dir = find_dir(cwd, path);
        }else
        {
            if(current_running->pfd[dirfd].used){
                dir.first_clus = current_running->pfd[dirfd].first_clus_num;
                dir = find_dir(dir, path);
            }else{
                kfree(buf);
                return -EBADF;
            }            
        }
    }
    if(dir.name[0] == 0xff)
    {
        printk("not find path!\n");
        kfree(buf);
        return -ENOENT;
    }else{
        if(search(dir.name, dir.first_clus, buf, FILE_DIR, NULL) == 0){
            memset(buf, 0, sizeof(PAGE_SIZE));
            create_new(dir.name, dir.first_clus, buf, FILE_DIR, NULL);
        } 
        kfree(buf);
        return 0;
    }
}   
int fat32_getdents(uint32_t fd, char *outbuf, uint32_t len)
{
    fd_t* nfd = &current_running->pfd[get_fd_index(fd, current_running)];
    // if(nfd->share_fd != 0 && nfd->share_fd->version > nfd->version){
    //     memcpy(nfd, nfd->share_fd, sizeof(fd_t));
    // }   
    // printk("fd: %d, getdents: %s, pos: %d, cur_clus: %d, cur_sec: %d\n",fd, nfd->name, nfd->pos, nfd->cur_clus_num, nfd->cur_sec);
    // uint32_t clus_num = nfd->first_clus_num;
    // uint32_t sec_num = first_sec_of_clus(clus_num);
    uint32_t read_len = 0;
    uint64_t d_ino;
    char *buf = kmalloc(PAGE_SIZE);
    // dentry_t *entry = buf;
    disk_read(buf, nfd->cur_sec, READ_MAX_SEC);
    dentry_t *entry = (dentry_t *)(buf + (nfd->pos % SECTOR_SIZE));
    int dirent64_size = sizeof(struct linux_dirent64);
    char filename[FAT32_MAX_FILENAME];
    int add_pos = 0;
    if (dentry_is_empty(entry) && !read_len)
    {
        // printk("getdents exit because of the entry is empty\n");
        kfree(buf);
        return 0;
    }
        
    while (dentry_is_empty(entry) == 0 && read_len < len)
    {
        memset(filename, 0, FAT32_MAX_FILENAME);
        while (entry->dename[0] == EMPTY_ENTRY)
        {
            // entry = next_entry(entry, buf, &clus_num, &sec_num);
            entry = next_entry(entry, buf, &nfd->cur_clus_num, &nfd->cur_sec);
            add_pos += sizeof(dentry_t);
            // printk("getdents: %s, pos: %d, cur_clus: %d, cur_sec: %d\n",nfd->name, nfd->pos, nfd->cur_clus_num, nfd->cur_sec);
        }
        if (dentry_is_empty(entry)) break;
        d_ino = nfd->cur_clus_num;
        
        long_name_entry_t *long_entry = (long_name_entry_t *)entry;
        //处理长目录项
        if(long_entry->attributes == ATTR_LONG_NAME && (long_entry->seq & LAST_LONG_ENTRY) == LAST_LONG_ENTRY)
        {
            uint16_t unicode;
            uint8 long_entry_num = long_entry->seq & LONG_ENTRY_SEQ;
            while (long_entry_num--)
            {
                int count = 0;
                for (int i = 0; i < LONG_FILENAME1; i++)
                {
                    unicode = long_entry->name1[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                for (int i = 0; i < LONG_FILENAME2; i++)
                {
                    unicode = long_entry->name2[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                for (int i = 0; i < LONG_FILENAME3; i++)
                {
                    unicode = long_entry->name3[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                // entry = next_entry(long_entry, buf, &clus_num, &sec_num);
                entry = next_entry((dentry_t *)long_entry, buf, &nfd->cur_clus_num, &nfd->cur_sec);
                add_pos += sizeof(long_name_entry_t);
                // printk("getdents: %s, pos: %d, cur_clus: %d, cur_sec: %d\n",nfd->name, nfd->pos, nfd->cur_clus_num, nfd->cur_sec);
                long_entry = (long_name_entry_t *)entry;
            }        
        }else{//短目录项
            int count = 0;
            for (int i = 0; i < SHORT_FILENAME; i++)
            {
                if(entry->dename[i] == ' ')
                    break;
                filename[count++] = entry->dename[i];
            }
            if(entry->extname[0] != ' ')
                filename[count++] = '.';
            for (int i = 0; i < SHORT_EXTNAME; i++)
            {
                if(entry->extname[i] == ' ')
                    break;
                filename[count++] = entry->extname[i];
            }
            filename[count] = 0;
        }//得到目录项记录的文件名字
        // printk("entry:%lx,name:%s, filename:%s\n",entry, entry->dename , filename);
        if(read_len + dirent64_size + strlen(filename) + 1 > len)
        {
            break;
        }
        struct linux_dirent64 *dirent = (struct linux_dirent64*)(outbuf + read_len);
        int last_read_len = read_len;
        read_len += dirent64_size + strlen(filename) + 1;
        read_len += (8 -(read_len%8));
        dirent->d_ino = d_ino;
        dirent->d_reclen = read_len - last_read_len;
        dirent->d_type = ((entry->arributes & ATTR_DIRECTORY) != 0) ? DT_DIR : DT_REG;
        memcpy(dirent->d_name, filename, strlen(filename) + 1);
        // entry = next_entry(entry, buf, &clus_num, &sec_num);
        entry = next_entry(entry, buf, &nfd->cur_clus_num, &nfd->cur_sec);
        add_pos += sizeof(dentry_t);
        // printk("filename: %s, linux dirent: %s\n",filename, dirent->d_name);
        // printk("getdents: %s, pos: %d, cur_clus: %d, cur_sec: %d\n",nfd->name, nfd->pos, nfd->cur_clus_num, nfd->cur_sec);
    }
    nfd->pos += add_pos;
    add_pos += (nfd->pos % SECTOR_SIZE);
    nfd->cur_sec += ((add_pos / SECTOR_SIZE) % READ_MAX_SEC);
    // printk("leave getdents: %s, pos: %d, cur_clus: %d, cur_sec: %d\n",nfd->name, nfd->pos, nfd->cur_clus_num, nfd->cur_sec);
    kfree(buf);
    return read_len;
}

int fat32_fstat(int fd, struct stat *statbuf)
{
    int fd_index = get_fd_index(fd, current_running);
    statbuf->st_dev = current_running->pfd[fd_index].dev;
    statbuf->st_ino = current_running->pfd[fd_index].first_clus_num;
    statbuf->st_mode = current_running->pfd[fd_index].flags;
    statbuf->st_nlink = current_running->pfd[fd_index].nlink;
    statbuf->st_uid = current_running->pfd[fd_index].uid;
    statbuf->st_gid = current_running->pfd[fd_index].gid;
    statbuf->st_rdev = current_running->pfd[fd_index].rdev;
    statbuf->st_size = current_running->pfd[fd_index].length;
    statbuf->st_blksize = fat.byts_per_clus;
    statbuf->st_blocks = current_running->pfd[fd_index].length / fat.byts_per_clus;
    statbuf->st_atim.tv_sec = current_running->pfd[fd_index].atime_sec;
    statbuf->st_mtim.tv_sec = current_running->pfd[fd_index].mtime_sec;
    statbuf->st_ctim.tv_sec = current_running->pfd[fd_index].ctime_sec;
    statbuf->st_atim.tv_nsec = 0;
    statbuf->st_mtim.tv_nsec = 0;
    statbuf->st_ctim.tv_nsec = 0;
    return 0;
}

int fat32_link()
{
    return -EPERM;
}

int fat32_unlink(int dirfd, const char* path, uint32_t flags)
{
    char *buf =kmalloc(PAGE_SIZE);
    // if((fd_index = get_fd_index(dirfd, current_running)) == -1)
    // {
    //     return -EBADF;
    // }
    // printk("path:%s\n", path);
    if(dirfd != AT_FDCWD)
    {
        kfree(buf);
        return -EBADF;
    }
    char path_copy[strlen(path) + 1];
    inode_t dir = {0};
    strcpy(path_copy, path);
    if(path[0] == '/')
    {
        dir = find_dir(root, path);
    }else{
        if(dirfd == AT_FDCWD)
        {
            dir = find_dir(cwd, path);
        }else
        {
            if(current_running->pfd[dirfd].used){
                dir.first_clus = current_running->pfd[dirfd].first_clus_num;
            }else{
                kfree(buf);
                return -EBADF;
            }
            dir = find_dir(dir, path);
        }
    }
    if(dir.name[0] == 0xff)
    {
        printk("not find path: %s!\n", path);
        kfree(buf);
        return -ENOENT;
    }
    type_t mode;
    mode = ((AT_REMOVEDIR & flags) == 0) ? FILE_FILE : FILE_DIR;
    struct dir_pos pos;
    // printk("dir.name:%s, dir.first clus: %d\n", dir.name, dir.first_clus);
    // printk("dir.name:%s, dir.first clus: %d\n", dir.name, dir.first_clus);
    dentry_t* dentry = search2(dir.name, dir.first_clus, buf, mode, &pos);
    if(dentry == NULL){
        printk("not find dir!\n");
        kfree(buf);
        return -ENOENT;
    }
    uint32_t sec = pos.sec;
    uint32_t clus = clus_of_sec(sec);
    dentry = (dentry_t *)(buf + pos.offset);
    // printk("pos.len: %d\n", pos.len);
    disk_read(buf, sec, READ_MAX_SEC);
    while (pos.len --)
    {
        dentry->dename[0] = EMPTY_ENTRY;
        if((dentry_t *)(dentry + 1) == (dentry_t *)(buf + BUFSIZE))
        {
            disk_write(buf, sec, READ_MAX_SEC);
        }
        dentry = next_entry(dentry, buf, &clus, &sec);
    }
    disk_write(buf, sec, READ_MAX_SEC);
    kfree(buf);
    return 0;
}
int fat32_dup(int src_fd){
    int ret_fd;
    // if (src_fd < 3) ret_fd = src_fd;
    //else {
        int32_t src_fd_index = get_fd_index(src_fd, current_running);
        if (src_fd_index == -1) ret_fd = -EBADF;
        else {
            ret_fd = get_one_fd(current_running);
            if (ret_fd == -1) {
                //printk("no fd remaining, return -EMFILE\n");
                return -EMFILE;
            }
            fd_t *nfd = &current_running->pfd[get_fd_index(ret_fd,current_running)];
            nfd->dev = current_running->pfd[src_fd_index].dev;
            nfd->redirected = 1;
            nfd->redirected_fd_index = src_fd_index;
        }
    //}
    // printk("[dup] try to dup %d, ret %d\n",src_fd,ret_fd);
    return ret_fd;
}
int fat32_dup3(int src_fd,int dst_fd){
    if (src_fd == dst_fd) return -EINVAL;
    int32_t src_fd_index = get_fd_index(src_fd, current_running);
    if (src_fd_index == -1) return -EBADF;
    int32_t dst_fd_index = get_fd_index(dst_fd, current_running);
    if (dst_fd_index != -1) fat32_close(dst_fd);
    // printk("[dup3] try to dup %d(%d), dst %d(%d)\n",src_fd,src_fd_index,dst_fd,dst_fd_index);
    int i;
    for (i=0;i<MAX_FILE_NUM;i++) {
        if (current_running->pfd[i].fd_num == dst_fd){
            current_running->pfd[i].used = 1;
            current_running->pfd[i].dev = current_running->pfd[src_fd_index].dev;
            current_running->pfd[i].redirected = 1;
            current_running->pfd[i].redirected_fd_index = src_fd_index;
            break;
        }
    }
    if (i == MAX_FILE_NUM){
        int ret_fd = get_one_fd(current_running);
        fd_t *nfd = &current_running->pfd[get_fd_index(ret_fd,current_running)];
        nfd->fd_num = dst_fd;
        nfd->redirected = 1;
        current_running->pfd[i].dev = current_running->pfd[src_fd_index].dev;
        nfd->redirected_fd_index = src_fd_index;
    }
    return dst_fd;
} 

/* =======final competition======== */
int64_t fat32_fcntl(fd_num_t fd, int32_t cmd, uint64_t arg){
    int32_t fd_index;
    if ((fd_index = get_fd_index(fd, current_running)) == -1) return -EBADF;
    if (cmd == F_GETFL) return current_running->pfd[fd_index].flags;
    else if (cmd == F_GETFD) return current_running->pfd[fd_index].mode;
    else if (cmd == F_SETFL) current_running->pfd[fd_index].flags = arg;
    else if (cmd == F_SETFD) current_running->pfd[fd_index].mode = arg;
    else if (cmd == F_SETLK) memcpy((char *)&current_running->pfd[fd_index].flock,(const char *)arg,sizeof(flock_t));
    else if (cmd == F_GETLK) memcpy((char *)arg,(const char *)&current_running->pfd[fd_index].flock,sizeof(flock_t));
    // dup function is in dup / dup3, for here, it is not implied
    return 0;
}

int64 fat32_readv(fd_num_t fd, struct iovec *iov, int iovcnt){
    int64_t tot_count = 0 ,cur_count = 0;
    for (uint32_t i = 0; i < iovcnt; i++){
    // printk("[readv] try to readv fd = %d, iov_base = 0x%x, iov_len = %d, iovcnt = %d\n",fd,iov->iov_base,iov->iov_len,iovcnt);
        if (iov->iov_len != 0) cur_count = fat32_read(fd, iov->iov_base, iov->iov_len);
        if ((cur_count) < 0) return cur_count;
        tot_count += cur_count;
        iov++;
    }
    return tot_count;
}
int64 fat32_writev(fd_num_t fd, struct iovec *iov, int iovcnt){
    int64_t tot_count = 0 ,cur_count = 0;
    for (uint32_t i = 0; i < iovcnt; i++){
        if (iov->iov_len != 0) cur_count = fat32_write(fd, iov->iov_base, iov->iov_len);
        if ((cur_count) < 0) return cur_count;
        tot_count += cur_count;
        iov++;
    }
    return tot_count;
}

#define AT_EMPTY_PATH 0x1000
int fat32_fstatat(fd_num_t dirfd, const char *pathname, struct stat *statbuf, int32 flags){

    // printk("[fstatat] enter fstatat with %s\n", pathname);
    //mode IGNORED
    int exist = 0;
    int i,fd;
    char *file_name;
    if (flags & AT_EMPTY_PATH)
    {
        fat32_fstat(dirfd,statbuf);
        return 0;        
    }
    // check if the target fd has been opened 
    // (we assume that there is no files which have the same name but in different directory)
    for (file_name = (char *)pathname; *file_name!='\0'; file_name++);
    for (; file_name != pathname && *file_name != '/'; file_name--);
    if (*file_name=='/') file_name++;
    for (i=0; i<MAX_FILE_NUM; i++){
        if (!strcmp(current_running->pfd[i].name,file_name)) {
            exist = 1;
            break;
        }
    }
    if (exist) {
        fd = current_running->pfd[i].fd_num;
        fat32_fstat(fd,statbuf);
        return 0;
    }
    else {
        if ((fd = fat32_openat(dirfd, pathname, O_RDONLY, flags)) >= 0) {
            fat32_fstat(fd,statbuf);
            // fat32_close(fd);
            return 0;
        }
        else if ((fd = fat32_openat(dirfd, pathname, O_DIRECTORY, flags)) >= 0) {
            fat32_close(fd);
            return -EISDIR;
        }
        else return -ENOENT;
    }
}

int fat32_statfs(const char *path, struct statfs *buf){
    buf->f_type = MSDOS_SUPER_MAGIC;
    buf->f_bsize = fat.byts_per_clus;
    buf->f_blocks = fat.data_clus_cnt;
    buf->f_bfree = fat.data_clus_cnt - fat.next_free;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = 10;
    buf->f_ffree = 10;
    buf->f_fsid.val[0] = 2064;
    buf->f_fsid.val[1] = 0;
    buf->f_namelen = 1530;
    return 0;
}
int fat32_fstatfs(int fd, struct statfs *buf){
    buf->f_type = MSDOS_SUPER_MAGIC;
    buf->f_bsize = fat.byts_per_clus;
    buf->f_blocks = fat.data_clus_cnt;
    buf->f_bfree = fat.data_clus_cnt - fat.next_free;
    buf->f_bavail = buf->f_bfree;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_fsid.val[0] = 2064;
    buf->f_fsid.val[1] = 0;
    buf->f_namelen = 1530;
    return 0;
}
int32_t fat32_utimensat(fd_num_t dirfd, const char *pathname, const struct timespec times[2], int32_t flags){
    int fd;
    int futimens = 0;
    struct timespec tmp_time = {0,0};
    //mode IGNORED
    if (pathname == NULL){
        futimens = 1;
        fd = dirfd;
        // printk("[utimensat] set time for fd %d\n",dirfd);
    }

    // else printk("[utimensat] set time for %s\n",pathname);
    if (futimens || (fd = fat32_openat(dirfd, pathname, O_RDONLY, flags)) >= 0 || (fd = fat32_openat(dirfd, pathname, O_DIRECTORY, flags)) >= 0) {
        if (fd < 0)
            return -ENOENT;
        int32_t fd_index = get_fd_index(fd, current_running);
        // printk("[utimensat] find fd %d, fd_index = %d\n",fd,fd_index);
        if (fd_index == -1) return -EBADF;
        if (times == NULL || times[0].tv_nsec == UTIME_NOW) {
            do_gettimeofday((timeval_t *)&tmp_time);
            memcpy((char *)&current_running->pfd[fd_index].atime_sec,(char *)&tmp_time,sizeof(struct timespec));
        }
        else if (times[0].tv_nsec != UTIME_OMIT) 
            memcpy((char *)&current_running->pfd[fd_index].atime_sec,(char *)&times[0],sizeof(struct timespec));
        if (times == NULL || times[1].tv_nsec == UTIME_NOW) {
            do_gettimeofday((timeval_t *)&tmp_time);
            memcpy((char *)&current_running->pfd[fd_index].mtime_sec,(char *)&tmp_time,sizeof(struct timespec));
        }
        else if (times[1].tv_nsec != UTIME_OMIT) 
            memcpy((char *)&current_running->pfd[fd_index].mtime_sec,(char *)&times[1],sizeof(struct timespec));
        if (!futimens) fat32_close(fd);
        return 0;
    }
    else return fd; // the dir or file is not found, cause is saved in fd
}
ssize_t fat32_pread(int fd, void * buf, size_t count, off_t offset){
    int64 backup = fat32_lseek(fd,0,SEEK_CUR); 
    fat32_lseek(fd,offset,SEEK_SET);
    int64 readlen = fat32_read(fd,buf,count);
    fat32_lseek(fd,backup,SEEK_SET);
    return readlen;
}

ssize_t fat32_pwrite(int fd, void * buf, size_t count, off_t offset){
    int64 backup = fat32_lseek(fd,0,SEEK_CUR); 
    fat32_lseek(fd,offset,SEEK_SET);
    int64 writelen = fat32_write(fd,buf,count);
    fat32_lseek(fd,backup,SEEK_SET);
    return writelen;
}

int32_t fat32_faccessat(fd_num_t dirfd, const char *pathname, 
                        int mode, int flags){
    int32_t fd;
    // for ls
    if ((fd = fat32_openat(dirfd, pathname, flags, mode)) >= 0)
    {
        fat32_close(fd);
        return 0;
    }
    else return fd;
}
int32_t fat32_fsync(fd_num_t fd){
    bsync();
    return 0;
}
size_t fat32_readlinkat(fd_num_t dirfd, const char *pathname, 
                        char *buf, size_t bufsiz){
    // printk("dirfd: %d; pathname: %s, buf:%lx\n", dirfd, pathname, buf);
    // char path[25] = "/lmbench_all";
    if (!strcmp(pathname, "/proc/self/exe")){
        if (current_running->name[0]!='/')
            strcpy(buf, "/");
        strcat(buf, current_running->name);
        return strlen(buf);
    }

    assert(0);

    return 0;
}


size_t fat32_sendfile(int out_fd, int in_fd, off_t *offset, size_t count){
    //out fd
    int out_fd_index = get_fd_index(out_fd, current_running);
    if (out_fd_index < 0) return -EBADF;
    fd_t __maybe_unused *out_nfd = &current_running->pfd[out_fd_index];
    //in fd
    int in_fd_index = get_fd_index(in_fd, current_running);
    if (in_fd_index < 0) return -EBADF;
    fd_t __maybe_unused *in_nfd = &current_running->pfd[in_fd_index];
    int in_fd_old_offset = 0;
    char *buff = kmalloc(PAGE_SIZE);
    int write_count = 0;
    if(offset){
        in_fd_old_offset = fat32_lseek(in_fd, 0, SEEK_CUR);
        fat32_lseek(in_fd, *offset, SEEK_SET);
    }
    while (write_count < count){
        int read_count = fat32_read(in_fd, buff, NORMAL_PAGE_SIZE);
        if (read_count <= 0) break;
        fat32_write(out_fd, buff, read_count);
        write_count += read_count; 
    }
    if (offset){
        fat32_lseek(in_fd, in_fd_old_offset, SEEK_SET);
        *offset += write_count;
    }
    kfree(buff);
    return write_count;
}

int fat32_renameat2(fd_num_t olddirfd, const char *oldpath_const, fd_num_t newdirfd, 
                    const char *newpath_const, unsigned int flags){
    assert(flags == 0);
    //search old path
    char *old_buf =kmalloc(PAGE_SIZE);
    if(olddirfd != AT_FDCWD)
    {
        kfree(old_buf);
        return -EBADF;
    }
    char old_path_copy[strlen(oldpath_const) + 1];
    inode_t old_dir = {0};
    strcpy(old_path_copy, oldpath_const);
    if(oldpath_const[0] == '/')
    {
        old_dir = find_dir(root, oldpath_const);
    }else{
        if(olddirfd == AT_FDCWD)
        {
            old_dir = find_dir(cwd, oldpath_const);
        }else
        {
            if(current_running->pfd[olddirfd].used){
                old_dir.first_clus = current_running->pfd[olddirfd].first_clus_num;
            }else{
                kfree(old_buf);
                return -EBADF;
            }
            old_dir = find_dir(old_dir, oldpath_const);
        }
    }
    if(old_dir.name[0] == 0xff)
    {
        printk("not find path: %s!\n", oldpath_const);
        kfree(old_buf);
        return -ENOENT;
    }
    
    struct dir_pos old_pos;
    // printk("dir.name:%s, dir.first clus: %d\n", dir.name, dir.first_clus);
    // printk("dir.name:%s, dir.first clus: %d\n", dir.name, dir.first_clus);
    dentry_t* old_dentry = search2(old_dir.name, old_dir.first_clus, old_buf, FILE_FILE, &old_pos);
    if(old_dentry == NULL){
        old_dentry = search2(old_dir.name, old_dir.first_clus, old_buf, FILE_DIR, &old_pos);
    }
    if(old_dentry == NULL){
        printk("not find dir!\n");
        kfree(old_buf);
        return -ENOENT;
    }    
    old_dentry = (dentry_t *)(old_buf + old_pos.offset);

    //search new path
    char *new_buf =kmalloc(PAGE_SIZE);
    if(newdirfd != AT_FDCWD)
    {
        kfree(old_buf);
        kfree(new_buf);
        return -EBADF;
    }
    char new_path_copy[strlen(newpath_const) + 1];
    inode_t new_dir = {0};
    strcpy(new_path_copy, newpath_const);
    if(newpath_const[0] == '/')
    {
        new_dir = find_dir(root, newpath_const);
    }else{
        if(newdirfd == AT_FDCWD)
        {
            new_dir = find_dir(cwd, newpath_const);
        }else
        {
            if(current_running->pfd[newdirfd].used){
                new_dir.first_clus = current_running->pfd[newdirfd].first_clus_num;
            }else{
                kfree(old_buf);
                kfree(new_buf);
                return -EBADF;
            }
            new_dir = find_dir(new_dir, newpath_const);
        }
    }
    if(new_dir.name[0] == 0xff)
    {
        printk("not find path: %s!\n", newpath_const);
        kfree(old_buf);
        kfree(new_buf);
        return -ENOENT;
    }
    
    struct dir_pos new_pos;
    // printk("dir.name:%s, dir.first clus: %d\n", dir.name, dir.first_clus);
    // printk("dir.name:%s, dir.first clus: %d\n", dir.name, dir.first_clus);
    dentry_t* new_dentry = search2(new_dir.name, new_dir.first_clus, new_buf, FILE_FILE, &new_pos);
    if(new_dentry == NULL){
        new_dentry = search2(new_dir.name, new_dir.first_clus, new_buf, FILE_DIR, &new_pos);
    }
    if(new_dentry != NULL){
        //if find, delete it
        uint32_t new_sec = new_pos.sec;
        uint32_t new_clus = clus_of_sec(new_sec);
        new_dentry = (dentry_t *)(new_buf + new_pos.offset);
        disk_read(new_buf, new_sec, READ_MAX_SEC);
        while (new_pos.len --)
        {
            new_dentry->dename[0] = EMPTY_ENTRY;
            if((uint64_t)(new_dentry + 1) == (uint64_t)(new_buf + BUFSIZE))
            {
                disk_write(new_buf, new_sec, READ_MAX_SEC);
            }
            new_dentry = next_entry(new_dentry, new_buf, &new_clus, &new_sec);
        }
        disk_write(new_buf, new_sec, READ_MAX_SEC);
    }
    //make a new
    memset(new_buf, 0, PAGE_SIZE);
    new_dentry = create_new(new_dir.name, new_dir.first_clus, new_buf, 
                 (old_dentry->arributes==ATTR_DIRECTORY)? FILE_DIR:FILE_FILE, NULL);

    //copy file from old to new
    int copy_count = 0;
    int file_sec = old_dentry->file_size/SECTOR_SIZE + (old_dentry->file_size % SECTOR_SIZE == 0)?0:1;
    char*copy_buf = kmalloc(PAGE_SIZE);
    uint32_t old_file_clus = get_cluster_from_dentry(old_dentry);
    uint32_t new_file_clus = get_cluster_from_dentry(new_dentry);
    uint32_t old_sec = first_sec_of_clus(old_file_clus);
    uint32_t new_sec = first_sec_of_clus(new_file_clus);
    while(file_sec > copy_count){
        int read_count = copy_count % SEC_PER_CLU;
        disk_read(copy_buf, old_sec + read_count, 1);
        disk_write(copy_buf, new_sec + read_count, 1);
        copy_count++;
        if(copy_count % SEC_PER_CLU == 0){
            old_file_clus = next_cluster(old_file_clus);
            new_file_clus = next_cluster(new_file_clus);
            old_sec = first_sec_of_clus(old_file_clus);
            new_sec = first_sec_of_clus(new_file_clus);
        }
    }
    kfree(copy_buf);
    
    //delete old
    old_sec = old_pos.sec;
    uint32_t old_clus = clus_of_sec(old_sec);
    disk_read(old_buf, old_sec, READ_MAX_SEC);
    while (old_pos.len --)
    {
        old_dentry->dename[0] = EMPTY_ENTRY;
        if((uint64_t)(old_dentry + 1) == (uint64_t)(old_buf + BUFSIZE))
        {
            disk_write(old_buf, old_sec, READ_MAX_SEC);
        }
        old_dentry = next_entry(old_dentry, old_buf, &old_clus, &old_sec);
    }
    disk_write(old_buf, old_sec, READ_MAX_SEC);
    kfree(old_buf);
    kfree(new_buf);
    return 0;
}

// only used when fast-loading elf 
int64 fat32_read_uncached(fd_num_t fd, char *buf, size_t count)
{
    // for (uint64_t i = (uint64_t)buf; i < (uint64_t)buf + count; i+=PAGE_SIZE)
    //     handle_page_fault_store(NULL, i, NULL);
    
    // printk("[read] fd:%d,buf:%lx, count: %d(0x%x)\n", fd, buf,count, count);
    // get nfd now
    int fd_index = get_fd_index(fd, current_running);
    if (fd_index == -1) return -EBADF;
    fd_t *nfd = &current_running->pfd[fd_index];
    //update fd
    // if(nfd->share_fd != 0 && nfd->share_fd->version > nfd->version){
    //     memcpy(nfd, nfd->share_fd, sizeof(fd_t));
    // }   
    // printk("[read] try to read fd = %d(index = %d), buf = 0x%x, count = %d\n",fd,fd_index,buf,count);
    assert(nfd != NULL);

    // handle "/etc/passwd", "/etc/localtime", DEV_STDIN, DEV_STDOUT
    // DEV_STDERR, DEV_NULL, DEV_ZERO
    int special_return = read_special_path(nfd, buf, count);
    // make sure not return -1 for pip_read
    if (special_return != -1)
        return special_return;
    
    // permission deny
    if (nfd->flags & O_DIRECTORY || nfd->flags & O_WRONLY) 
        return -EINVAL;
    char * buffer = kmalloc(PAGE_SIZE);
    char * buf_point = buf;
    // debug_print_fd(nfd);
    if (nfd->length < nfd->pos) {
        kfree(buffer);
        return -EFAULT;
    }
    uint32_t real_count = min(count, nfd->length - nfd->pos);
    // printk("[read] nfd->length = %d, nfd->pos = %d\n",nfd->length,nfd->pos);
    uint32_t ncount = 0;
    /* record */
    uint32_t old_clus_num = nfd->pos / CLUSTER_SIZE;
    uint32_t old_sec_num = nfd->pos / SECTOR_SIZE;
    uint32_t new_clus_num = old_clus_num;
    uint32_t new_sec_num = old_sec_num;
    while (ncount < real_count) {
        // change in 2022 7.16
        // size_t pos_offset_in_buf = nfd->pos % BUFSIZE;
        size_t pos_offset_in_buf = nfd->pos % SECTOR_SIZE;
        // printk("pos_offset_in_buf: %d\n", pos_offset_in_buf);
        size_t readsize = min(BUFSIZE - pos_offset_in_buf, real_count - ncount);
        /* read readsize */
        // printk("fat32_read: 0x%lx,%d,%d\n",buffer, nfd->cur_sec, READ_MAX_SEC);
        sd_read_sector(buffer, nfd->cur_sec, READ_MAX_SEC);
        /* memcpy */
        memcpy(buf_point, buffer + pos_offset_in_buf, readsize); 
        buf_point += readsize;
        ncount += readsize;
        nfd->pos += readsize;
        new_clus_num = nfd->pos / CLUSTER_SIZE;
        new_sec_num = nfd->pos / SECTOR_SIZE;
        if (new_clus_num - old_clus_num) {
            char *clus_buf = kmalloc(PAGE_SIZE);
            sd_read_sector(clus_buf, fat_sec_of_clus(nfd->cur_clus_num, 1), 1);
            uint32 offset = fat_offset_of_clus(nfd->cur_clus_num);
            nfd->cur_clus_num  = *(uint32_t*)(clus_buf + offset);
            kfree(clus_buf);
            nfd->cur_sec = ((first_sec_of_clus(nfd->cur_clus_num) + \
                            (nfd->pos % CLUSTER_SIZE) / SECTOR_SIZE));
        } else 
        {
            if (new_sec_num - old_sec_num) 
                nfd->cur_sec += (new_sec_num - old_sec_num);
        }
        old_clus_num = new_clus_num;
        old_sec_num = new_sec_num;
    }
    kfree(buffer);
    // printk("[read] read; fd = %d(%d), buf = 0x%lx, real_count = %d\n",fd,fd_index,buf,real_count);
    // nfd->version++;
    // memcpy(nfd->share_fd, nfd, sizeof(fd_t));
    return real_count;
}