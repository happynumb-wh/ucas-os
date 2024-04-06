#include <fs/fat32.h>
#include <fs/pipe.h>
#include <os/sched.h>
#include <stdio.h>

pipe_t pipe[PIPE_NUM];
/*
 * @brief  pipe initialization.
 */
void init_pipe(){
    for (int i=0; i<PIPE_NUM; i++){
        init_list(&pipe[i].r_list);
        init_list(&pipe[i].w_list);
        //do_mutex_lock_init(&pipe[i].lock);
        init_ring_buffer(&pipe[i].rbuf);
        pipe[i].r_valid = 0;
        pipe[i].w_valid = 0;
    }
}
/**
 * @brief  Read "count" byte from the pipe.
 *         If the pipe is empty, block current process in pipe's r_list.
 *         Otherwise, read the buffer and return actually read size.
 *         Attention: maybe execute this function once can not transfer all needed data from pipe.
 * @param  buf: dst buffer.
 * @param  pipe_num: src pipe number.
 * @param  count: byte num read from pipe.
 * @retval  -1:  failed
 *           0:  read unavailable
 *           others: actually read size 
 */
int32_t pipe_read(char *buf, uint32_t pip_num, size_t count){
    //do_mutex_lock_acquire(&pipe[pip_num].lock);
    if (count == 0) return 0;
    if (!pipe[pip_num].r_valid) {
        //do_mutex_lock_release(&pipe[pip_num].lock);
        return -EINVAL;
    }
    else {
        int32_t r_size;
        while (!(r_size = read_ring_buffer(&pipe[pip_num].rbuf,buf,count))){
            if (!pipe[pip_num].w_valid) break;
            else {
                do_block(&current_running->list,&pipe[pip_num].r_list);
                //do_mutex_lock_release(&pipe[pip_num].lock);
            }
        }
        while (r_size && !list_empty(&pipe[pip_num].w_list)) do_unblock(pipe[pip_num].w_list.next);
        //do_mutex_lock_release(&pipe[pip_num].lock);
        return r_size;
    }
}
/**
 * @brief  Write "count" byte to the pipe.
 *         If the pipe is full, block current process in pipe's w_list.
 *         Otherwise, write the buffer and return actually written size.
 *         Attention: maybe execute this function once can not transfer all needed data to pipe.
 * @param  buf: src buffer.
 * @param  pipe_num: dst pipe number.
 * @param  count: byte num write to pipe.
 * @retval -1: failed
 *          0: succeed
 *          others: actually write size 
 */
int32_t pipe_write(const char *buf, uint32_t pip_num, size_t count){
    //do_mutex_lock_acquire(&pipe[pip_num].lock);
    if (!pipe[pip_num].w_valid) {
        //do_mutex_lock_release(&pipe[pip_num].lock);
        return -EINVAL;
    }
    else {
        int32_t w_size;
        while (!(w_size = write_ring_buffer(&pipe[pip_num].rbuf,buf,count))){
            if (!pipe[pip_num].r_valid) {
                printk("[SIGNAL] SIGPIPE\n");
                break;
            }
            else {
                do_block(&current_running->list,&pipe[pip_num].w_list);
                //do_mutex_lock_release(&pipe[pip_num].lock);
            }
        }
        while (w_size && !list_empty(&pipe[pip_num].r_list)) do_unblock(pipe[pip_num].r_list.next);
        //do_mutex_lock_release(&pipe[pip_num].lock);
        return w_size;
    }
}
/*
 * @brief  decrease pipe's r_valid counter.
 * @param  pipe_num: dst pipe number.
 * @retval -1: failed
 *          0: succeed
 */
int32_t close_pipe_read(uint32_t pip_num){
   // do_mutex_lock_acquire(&pipe[pip_num].lock);
    if (!pipe[pip_num].r_valid) {
        //do_mutex_lock_release(&pipe[pip_num].lock);
        return -EINVAL;
    }
    else {
        pipe[pip_num].r_valid--;
        //do_mutex_lock_release(&pipe[pip_num].lock);
        return 0;
    }
}
/*
 * @brief  decrease pipe's w_valid counter.
 * @param  pipe_num: dst pipe number.
 * @retval -1: failed
 *          0: succeed
 */
int32_t close_pipe_write(uint32_t pip_num){
    //do_mutex_lock_acquire(&pipe[pip_num].lock);
    if (!pipe[pip_num].w_valid) {
        //do_mutex_lock_release(&pipe[pip_num].lock);
        return -EINVAL;
    }
    else {
        pipe[pip_num].w_valid--;
        //do_mutex_lock_release(&pipe[pip_num].lock);
        return 0;
    }
}
/**
 * @brief alloc one point pipe
 */
int32_t alloc_one_pipe()
{
    uint32_t pipenum;
    //alloc pipe
    int i;
    for (i = 0;i < PIPE_NUM; i++) {
        if (!pipe[i].r_valid && !pipe[i].w_valid) {
        pipe[i].r_valid = 1;
        pipe[i].w_valid = 1;
        pipe[i].pid = current_running->pid;
        pipenum = i;
        break;
        }
    }
    if (i==PIPE_NUM) return -ENFILE;
    return pipenum;
}
/*
 * @brief  allocate pipe to fd[2].
 * @param  fd: target fd array.
 * @retval  -1: failed
 *          0: succeed
 */
int32_t fat32_pipe2(uint32_t *fd){
    int i;
    uint32_t pipenum;
    //alloc pipe
    for (i=0;i<PIPE_NUM;i++) {
        //do_mutex_lock_acquire(&pipe[i].lock);
         if (!pipe[i].r_valid && !pipe[i].w_valid) {
            pipe[i].r_valid = 1;
            pipe[i].w_valid = 1;
            pipe[i].pid = current_running->pid;
            pipenum = i;
            //do_mutex_lock_release(&pipe[i].lock);
            break;
         }
         //do_mutex_lock_release(&pipe[i].lock);
    }
    if (i==PIPE_NUM) return -ENFILE;
    //alloc 2 fds
    fd[0] = get_one_fd(current_running);
    fd_t *fd_0 = &current_running->pfd[get_fd_index(fd[0],current_running)];
    fd_0->dev = DEV_PIPE;
    fd_0->pip_num = pipenum;
    fd_0->is_pipe_read = 1;
    fd_0->is_pipe_write = 0;
    fd[1] = get_one_fd(current_running);
    fd_t *fd_1 = &current_running->pfd[get_fd_index(fd[1],current_running)];
    fd_1->dev = DEV_PIPE;
    fd_1->pip_num = pipenum;
    fd_1->is_pipe_read = 0;
    fd_1->is_pipe_write = 1;
    // init_share_fd(fd_0);
    // init_share_fd(fd_1);
    // printk("[pipe] alloc fd0: %d, fd1: %d\n", *fd, *(fd+1));
    return 0;
}

