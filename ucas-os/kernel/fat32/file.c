#include <fs/file.h>
#include <os/time.h>
#include <os/sched.h>
#include <fs/pipe.h>

pfd_table_t pfd_table[NUM_MAX_TASK];

/**
 * @brief 初始化所有的文件描述符，归内核进行管理，以链表的方式进行管理
 * @param fd 文件描述符
 */
void init_fd(fd_t *fd)
{
    fd[0].dev = DEV_STDIN; fd[0].fd_num = 0; fd[0].used = FD_USED;
    fd[1].dev = DEV_STDOUT;fd[1].fd_num = 1; fd[1].used = FD_USED; 
    fd[2].dev = DEV_STDERR;fd[2].fd_num = 2; fd[2].used = FD_USED;
    // fd[0].share_fd = 0; fd[0].version = 0; fd[0].share_num = 0;
    // fd[1].share_fd = 0; fd[1].version = 0; fd[1].share_num = 0;
    // fd[2].share_fd = 0; fd[2].version = 0; fd[2].share_num = 0;
    for (int i = 3; i < MAX_FD_NUM; i++)
    {
        /* default */
        fd[i].dev = DEV_DEFAULT;
        fd[i].fd_num = i;
        fd[i].used = FD_UNUSED; 
        // fd[i].version = 0;
        // fd[i].share_num = 0;
    }
}
/**
 * @brief clone, fork操作
 * @param dst_fd 目的fd描述符
 * @param src_fd 源描述符
 */
void copy_fd(fd_t *dst_fd, fd_t *src_fd){
    if (src_fd->used && src_fd->dev == DEV_PIPE){
        if (src_fd->is_pipe_read) pipe[src_fd->pip_num].r_valid++;
        else if (src_fd->is_pipe_write) pipe[src_fd->pip_num].w_valid++;
    }
    // if (src_fd->share_fd)
    //     src_fd->share_fd->share_num++;
    memcpy((void *)dst_fd, (void *)src_fd, sizeof(fd_t));
}
/**
 * @brief 从指定的pcb处得到一个空闲的fd
 * @param pcb 对应的pcb指针
 * @return 成功返回描述下标，失败返回-1
 */
uint32_t get_one_fd(void * pcb)
{
    int32_t alloc_fd = 0;
    int ok = 0;
    pcb_t * under_pcb = (pcb_t*)pcb;
    while(!ok){
        ok = 1;
        for (int i = 0; i < MAX_FILE_NUM; i++)
            if (under_pcb->pfd[i].used && under_pcb->pfd[i].fd_num == alloc_fd) {
                ok = 0;
                // printk("alloc fd %d fail, fd_index = %d, under_pcb->pfd[i].used = %d\n",alloc_fd,i,under_pcb->pfd[i].used);
                alloc_fd++;
                break;
            }
    }
    // printk("alloc fd %d success\n",alloc_fd);
    for (int i = 0; i < MAX_FILE_NUM; i++)
        if (!under_pcb->pfd[i].used) {
            under_pcb->pfd[i].used = 1;
            under_pcb->pfd[i].fd_num = alloc_fd;
            return alloc_fd;
        } 
    return -1;
}
/**
 * @brief 更具传入的fd值，找到对应的进程下的fd的真实数组下标
 * @param fd 文件描述符的值
 * @param pcb pcb指针
 */
int32_t get_fd_index(fd_num_t fd, void * pcb)
{
    pcb_t * under_pcb = (pcb_t*)pcb;
    for (int i = 0; i < MAX_FILE_NUM; i++)
    {
        if (under_pcb->pfd[i].used && under_pcb->pfd[i].fd_num == fd) {
            while (under_pcb->pfd[i].dev == DEV_DEFAULT && under_pcb->pfd[i].redirected) i = under_pcb->pfd[i].redirected_fd_index;
            if (under_pcb->pfd[i].dev == DEV_DEFAULT && under_pcb->pfd[i].used == 0) return -1;
            else return i;
        }
    }
    return -1;
}

int32_t get_fd_index_without_redirect(fd_num_t fd, void * pcb)
{
    pcb_t * under_pcb = (pcb_t*)pcb;
    for (int i = 0; i < MAX_FILE_NUM; i++)
        if (under_pcb->pfd[i].used && under_pcb->pfd[i].fd_num == fd) return i;
    return -1;
}

// fd_t fd_share[16] = {0};

// void init_share_fd(fd_t *fd){
//     fd_t* share_fd = kmalloc(PAGE_SIZE);
//     char *c = share_fd;
//     fd->share_fd = share_fd;
//     memcpy(share_fd, fd, sizeof(fd_t));
//     share_fd->share_num++;
// }

void init_fd_table(){
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        pfd_table[i].num = 0;
        pfd_table[i].used = 0;
    }
}
/**
 * @brief alloc a fd_table to a process
 * 
 * @return fd_t* pfd_table pointer;
 * 
 */
fd_t *alloc_fd_table(){
    for (int i = 0; i < NUM_MAX_TASK; i++)
    {
        if(pfd_table[i].used == 0){
            // printk("get: %d\n", i);
            pfd_table[i].used = 1;
            pfd_table[i].num = 1;
            return pfd_table[i].pfd;
        }
    }
    printk("fd_table is full\n");
    assert(0);
    return NULL;
}

void free_fd_table(fd_t* pfd_in){
    pfd_table_t* pt =  list_entry(pfd_in, pfd_table_t, pfd);  
    pt->num --;
    if(pt->num == 0){
        pt->used = 0;
        for (int i = 0;i < MAX_FD_NUM;i++){
            fd_t *nfd = &(pfd_in[i]);
            if (nfd->dev == DEV_PIPE){
                if (nfd->is_pipe_read) close_pipe_read(nfd->pip_num);
                else if (nfd->is_pipe_write) close_pipe_write(nfd->pip_num);
            }
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
        }
    }
    // printk("free pt:%lx, used: %d, num: %d\n", pt, pt->used, pt->num);
    // for (int i = 0; i < NUM_MAX_TASK; i++)
    // {
    //    printk("%d: %lx pt_used: %d\n", i, &pfd_table[i], pfd_table[i].used);
    // }    
}

void update_fd_length(fd_t *src_pfd){
    for (int i = 0;i < NUM_MAX_TASK; i++){
        if (pfd_table[i].used)
            for (int j=0; j<MAX_FILE_NUM; j++){
                if ((pfd_table[i].pfd[j].used) && (&(pfd_table[i].pfd[j]) != src_pfd) &&
                    !strcmp(pfd_table[i].pfd[j].name,src_pfd->name)) 
                        pfd_table[i].pfd[j].length = src_pfd->length;
            }
    }
}