#include <os/io.h>
#include <screen.h>
/* success return 0, fail return 0 */
// what's the meaning of this syscall???
int64_t do_ioctl(fd_num_t fd, uint64_t request, const char *argp[])
{
    
    int32_t fd_index;
    if ((fd_index = get_fd_index(fd, current_running)) == -1)
        return -EBADF;
    else return 0;
}

/* success return 0, error return -1, timeout return 1 /=*/
static int handle_select(fd_num_t fd, int mode, uint64_t timeout_ticks)
{
    
    int32_t fd_index = get_fd_index(fd, current_running);
    if (fd_index < 0)
        return -1;

    fd_t *fdp = &current_running->pfd[fd_index];
    uint64_t final_ticks = get_ticks() + timeout_ticks;

    if (mode == SELECT_READ){
        if (fdp->dev == DEV_STDIN){            
            return wait_ring_buffer_read(&stdin_buf, final_ticks);;
        }
        else if (fdp->is_pipe_read || fdp->is_pipe_write){            
            return wait_ring_buffer_read(&(pipe[fdp->pip_num].rbuf), final_ticks);;
        }
        else
            assert(0);
    }
    else if (mode == SELECT_WRITE){
        if (fdp->dev == DEV_STDOUT){
            return wait_ring_buffer_write(&stdout_buf, final_ticks);
        }
        else if (fdp->dev == DEV_STDERR){
            return wait_ring_buffer_write(&stderr_buf, final_ticks);
        }
        else if (fdp->is_pipe_read || fdp->is_pipe_write){
            return wait_ring_buffer_write(&(pipe[fdp->pip_num].rbuf), final_ticks);
        }
        else
            return 0; /* Normal file is always OK to be written */
    }
    return -1;
}

static void clear_all_fds(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds)
{
    uint32_t fd;
    for (fd = 0; fd < nfds; fd++){
        if (readfds)
            FD_CLR(fd, readfds);
        if (writefds)
            FD_CLR(fd, writefds);
        if (exceptfds)
            FD_CLR(fd, exceptfds);
    }
}

int do_pselect6(int nfds, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, const struct timespec *timeout,
                   const sigset_t *sigmask){
    // printk("nfds:%d, readfds:%lx, writefds:%lx, exceptfds:%lx\n", nfds, readfds, writefds, exceptfds);
    if(nfds < 0)
        return -EINVAL;
    else if(nfds == 0)
        return 0;
    uint64_t timeout_ticks = timespec_to_ticks(timeout);
    uint32_t fd = 0;
    int rtn_num = 0;
    for (fd = 0; fd < nfds; fd++){
        if (readfds && FD_ISSET(fd, readfds)){
            int32_t ret;
            if ((ret = handle_select(fd, SELECT_READ, timeout_ticks)) < 0){
                return -1;
            }
            else if (ret == 0){
                rtn_num++;
                clear_all_fds(nfds, readfds, writefds, exceptfds);
                FD_SET(fd, readfds);
                break;
            }
            else{
                FD_CLR(fd, readfds);
            }
        }
        if (writefds && FD_ISSET(fd, writefds)){
            int32_t ret;
            if ((ret = handle_select(fd, SELECT_WRITE, timeout_ticks)) < 0){
                return -1;
            }
            else if (ret == 0){
                rtn_num++;
                clear_all_fds(nfds, readfds, writefds, exceptfds);
                FD_SET(fd, writefds);
                break;
            }
            else{
                FD_CLR(fd, writefds);
            }
        }
        if (exceptfds && FD_ISSET(fd, exceptfds))
            FD_CLR(fd, exceptfds);
    }
    return rtn_num;
    
}