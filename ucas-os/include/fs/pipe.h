#ifndef INCLUDE_PIPE_H
#define INCLUDE_PIPE_H

#include <type.h>
#include <os/list.h>
#include <os/lock.h>
#include <os/ring_buffer.h>

/* pipe */
#define PIPE_NUM 100
//#define PIPE_INVALID 0
//#define PIPE_VALID 1
//#define PIPE_ERROR 2

//#define FD_UNPIPED 0
//#define FD_PIPED 1

typedef struct pipe{
    //mutex_lock_t lock;
    struct ring_buffer rbuf;
    pid_t pid; // parent id
    list_head r_list;
    list_head w_list;
    uint8 r_valid;
    uint8 w_valid;
}pipe_t;

extern pipe_t pipe[PIPE_NUM];

void init_pipe();
int32_t close_pipe_read(uint32_t pip_num);
int32_t close_pipe_write(uint32_t pip_num);
int32_t fat32_pipe2(uint32_t *fd);
int32_t pipe_read(char *buf, uint32_t pip_num, size_t count);
int32_t pipe_write(const char *buf, uint32_t pip_num, size_t count);
int32_t alloc_one_pipe();
#endif
