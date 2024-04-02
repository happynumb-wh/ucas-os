#ifndef __FILE_H
#define __FILE_H
#include <type.h>
#include <os/list.h>
#include <os/spinlock.h>
#include <os/errno.h>
#include <os/system.h>

#define MAX_FD_NUM 110
#define MAX_FILE_NUM (mylimit.rlim_cur) // actually it is current_running->fd_limit.rlim_cur, but if so, the invalid float instruction panic will occurred due to unknown reason

#define O_RDONLY 0x00
#define O_WRONLY 0x01
#define O_RDWR 0x02 // 可读可写
//#define O_CREATE 0x200
#define O_CREATE 0x40
#define O_DIRECTORY 0x0200000

#define DEV_STDIN 0
#define DEV_STDOUT 1
#define DEV_STDERR 2
#define DEV_DEFAULT 3
#define DEV_NULL 4
#define DEV_ZERO 5
#define DEV_PIPE 6
#define DEV_SOCK 7
/* lseek */
#define SEEK_SET 0x0
#define SEEK_CUR 0x1
#define SEEK_END 0x2


// #define DIR 0x040000
// #define FILE 0x100000

#define AT_FDCWD -100

#define S_IFMT  0170000

#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFBLK 0060000
#define S_IFREG 0100000
#define S_IFIFO 0010000
#define S_IFLNK 0120000
#define S_IFSOCK 0140000

typedef struct dir_pos{
    uint64_t offset;
    uint32_t len;
    uint32_t sec;
}dir_pos_t;

enum{
    FD_UNUSED,
    FD_USED,
};

typedef enum{
    FILE_FILE,
    FILE_DIR,
}type_t;

typedef struct{
	short l_type;
	short l_whence;
	off_t l_start;
	off_t l_len;
	pid_t l_pid;
}flock_t;

typedef struct fd{
    /* file or dir name */
    char name[50];
    /* dev number */
    uint8 dev;
    /* first clus number */
    uint32 first_clus_num;
    /* file mode */
    uint32 mode;
    /* open flags */
    uint32 flags;
    /* position */
    uint64 pos;
    uint32 cur_sec;
    uint32 cur_clus_num;
    /* length */
    uint32 length;
    /* dir_pos */
    dir_pos_t dir_pos;

    /* fd number */
    /* default: its index in fd-array*/
    fd_num_t fd_num;

    /* used */
    uint8 used;
    /* redirect */
    uint8 redirected;
    uint8 redirected_fd_index;

    /* pipes[pip_num] is the pipe */
    pipe_num_t pip_num;
    uint8 is_pipe_read;
    uint8 is_pipe_write;
    /* socket */
    uint8 sock_num;
    /*mmap*/
    struct {
        int used;
        void *start;
        size_t len;
        int prot;
        int flags;
        off_t off;
    }mmap;
    /* fcntl */
    flock_t flock;
    /* links */
    uint8 nlink;

    /* status */
    uid_t uid;
    gid_t gid;

    dev_t rdev;

    blksize_t blksize;

    long atime_sec;
    long atime_nsec;
    long mtime_sec;
    long mtime_nsec;
    long ctime_sec;
    long ctime_nsec;

    //share fd
    // struct fd* share_fd;
    // int   share_num;
    // long version;
}fd_t;

typedef struct pfd_table{
    int used; 
    int num; // the number of process;
    fd_t pfd[MAX_FD_NUM];
}pfd_table_t;

/* init all fd */
void init_fd(fd_t *fd);
/* copy fd for the fork */
void copy_fd(fd_t *dst_fd, fd_t *src_fd);
/* get one free fd */
uint32_t get_one_fd(void * pcb);
/* get fd's index */
int32_t get_fd_index(fd_num_t fd, void * pcb);
int32_t get_fd_index_without_redirect(fd_num_t fd, void * pcb);
// void init_share_fd(fd_t* fd);
/* fd_table */
void init_fd_table();
fd_t *alloc_fd_table();
void free_fd_table(fd_t* pfd);
void update_fd_length(fd_t *src_pfd);
#endif