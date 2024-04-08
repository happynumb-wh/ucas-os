#ifndef __FAT32_H
#define __FAT32_H

#include <type.h>
#include <sdcard.h>
#include <os/time.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <os/sleeplock.h>
#include <fs/pipe.h>
#include <fs/file.h>
#include <stdio.h>
#include "assert.h"
/**
 * BPB offset
 */
#define BPB_BytsPerSec  11  // Bytes of every sector 512
#define BPB_SecPerClus  13  // sector nums of every clus
#define BPB_RsvdSexCnt  14  // num of reserved sec be 1 for the FAT12 and FAT16
#define BPB_NumFATs     16  // num of fat tables
#define BPB_RootEntCnt  17  // FAT32 be zero 
#define BPB_TotSec16    19  // FAT32 be zero
#define BPB_Media       21  // 
#define BPB_FATSz16     22  // FAT32 be zero
#define BPB_SecPerTrk   24  // Ignore
#define BPB_NumHeads    26  // Ignore
#define BPB_HiddSec     28  // Ignore
#define BPB_TocSec32    32  // Total sector Num
#define BPB_FATSz32     36  // one FAT table's sector num
#define BPB_ExtFlags    40  // Ignore
#define BPB_FSVer       42  // Version
#define BPB_RootClus    44  // root directory's clus
#define BPB_FSInfo      48  // info
#define BPB_BkBootSec   50  // Usually be 6
#define BPB_Reserved    52  // extention
#define BPB_DrvNum      64  // Ignore

#define AT_FDCWD        -100
#define AT_REMOVEDIR	0x200   /* Remove directory instead of
                               unlinking file.  */

#define SUBMIT
#ifndef SUBMIT
    #ifdef k210
        #define FAT_FIRST_SEC 2048
    #else
        #define FAT_FIRST_SEC 0
    #endif
#else
    #define FAT_FIRST_SEC 0
#endif
/**
 * BS
 * 
 */
#define BS_FileSysType  82  // usually be FAT32, usually not the judgement for the FAT32???

/* fcntl */
#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL	4
#define F_GETLK	5
#define F_SETLK 6

#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2


/**
 * FAT32 file attribute
 */
#define ATTR_READ_WRITE     0x00
#define ATTR_READ_ONLY      0x01
#define ATTR_HIDDEN         0x02
#define ATTR_SYSTEM         0x04
#define ATTR_VOLUME_ID      0x08
#define ATTR_DIRECTORY      0x10
#define ATTR_ARCHIVE        0x20
#define ATTR_LONG_NAME      0x0F

#ifndef max
#define max(x,y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef min
#define min(x,y) ((x)<(y) ? (x) : (y))
#endif

#define SECTOR_SIZE (fat.bpb.byts_per_sec)
#define CLUSTER_SIZE (fat.byts_per_clus)
#define READ_PAGE_CNT (NORMAL_PAGE_SIZE/fat.bpb.byts_per_sec)
#define READ_CLUS_CNT (CLUSTER_SIZE/fat.bpb.byts_per_sec)
#define BUFSIZE (min(NORMAL_PAGE_SIZE, CLUSTER_SIZE))
#define READ_MAX_SEC (BUFSIZE/SECTOR_SIZE)
#define SEC_PER_CLU fat.bpb.sec_per_clus
/**
 *  FAT32 flags
 */
#define FAT32_EOC           0x0ffffff8
#define M_FAT32_EOC         0x0FFFFFFF
#define FAT_MASK            0x0fffffffu
#define EMPTY_ENTRY         0xe5
#define END_OF_ENTRY        0x00
#define LAST_LONG_ENTRY		0x40
#define LONG_ENTRY_SEQ      0x0f      

#define CHAR_LONG_NAME		13
#define CHAR_SHORT_NAME		11
#define FAT32_MAX_FILENAME  256
#define FAT32_MAX_PATH      260
#define ENTRY_CACHE_NUM     50
#define SECSZ				512
#define FAT_CACHE_NSEC		(PGSIZE / SECSZ)
#define CLUSER_END          0x0fffffff
typedef struct fat32_sb{
    // sleeplist_t fat_lock;
    uint32_t  first_data_sec;
    uint32_t  data_sec_cnt;
    uint32_t  data_clus_cnt;
    uint32_t  byts_per_clus;
    uint32_t  free_count;		// of cluster
	uint32_t  next_free;		// clus
	uint16_t  fs_info;		    // fs info sector
	uint32_t  next_free_fat;	// the next fat sec that has room
    struct {
        uint16_t  byts_per_sec;
        uint8_t   sec_per_clus;
        uint16_t  rsvd_sec_cnt;
        uint8_t   fat_cnt;            /* count of FAT regions */
        uint32_t  hidd_sec;           /* count of hidden sectors */
        uint32_t  tot_sec;            /* total count of sectors including all regions */
        uint32_t  fat_sz;             /* count of sectors for a FAT region */
        uint32_t  root_clus;
    } bpb;
} fat_t;

typedef uint32_t isec_t;

#define SHORT_FILENAME    8
#define SHORT_EXTNAME     3
typedef struct short_name_entry{
    char        dename[SHORT_FILENAME];
    char        extname[SHORT_EXTNAME];
    uint8_t     arributes;
    uint8_t     Lowercase;
    uint8_t     creat_time_ms;//10ms为一单位
    uint16_t    creat_time;//0-4：秒，单位2, 5-10：分，11-15：时
    uint16_t    creat_date;//0-4：日，5-8：月，9-15：年
    uint16_t    last_visit_time;
    uint16_t    high_start_clus;
    uint16_t    modity_time;
    uint16_t    modity_date;
    uint16_t    low_start_clus;
    uint32_t    file_size;
} __attribute__((packed)) short_name_entry_t;
typedef short_name_entry_t dentry_t;


#define LONG_FILENAME1 5
#define LONG_FILENAME2 6
#define LONG_FILENAME3 2
#define LONG_FILENAME (LONG_FILENAME1 + LONG_FILENAME2 + LONG_FILENAME3)
typedef struct long_name_entry
{
    uint8_t seq;
    uint16_t name1[LONG_FILENAME1];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t checksum;
    uint16_t name2[LONG_FILENAME2];
    uint16_t start_clus;
    uint16_t name3[LONG_FILENAME3];
}__attribute__((packed)) long_name_entry_t ;

extern fat_t fat;
struct stat {
    dev_t     st_dev;         /* ID of device containing file */
    ino_t     st_ino;         /* inode number */
    mode_t    st_mode;        /* protection */
    nlink_t   st_nlink;       /* number of hard links */
    uid_t     st_uid;         /* user ID of owner */
    gid_t     st_gid;         /* group ID of owner */
    dev_t     st_rdev;        /* device ID (if special file) */
    uint64_t  __pad1;
    off_t     st_size;        /* total size, in bytes */
    blksize_t st_blksize;     /* blocksize for filesystem I/O */
    uint32_t  __pad2;
    blkcnt_t  st_blocks;      /* number of 512B blocks allocated */

    /* Since Linux 2.6, the kernel supports nanosecond
        precision for the following timestamp fields.
        For the details before Linux 2.6, see NOTES. */

    struct timespec st_atim;  /* time of last access */
    struct timespec st_mtim;  /* time of last modification */
    struct timespec st_ctim;  /* time of last status change */
    uint32_t __unused[2];
};

struct iovec{
    void *iov_base;
    size_t iov_len;
};

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14

struct linux_dirent64 {
    uint64        d_ino;
    int64         d_off;
    unsigned short  d_reclen;
    unsigned char   d_type;
    char           d_name[];
};

// typedef struct dir_pos{
//     uint64_t offset;
//     size_t len;
//     uint32_t sec;
// }dir_pos_t;

typedef struct inode{
    uint32_t first_clus;
    uint32_t clus_num;
    uint32_t sector_num;
    char name[FAT32_MAX_PATH];//可作为路径
}inode_t;

// 给定簇号的第一个扇区的位置
static inline uint32 first_sec_of_clus(uint32 cluster)
{
    return ((cluster - 2) * fat.bpb.sec_per_clus) + fat.first_data_sec;
}

// 由扇区号得到簇号
static inline uint32 sec_to_clus(uint32_t sec_num)
{
    return (sec_num - fat.first_data_sec) / fat.bpb.sec_per_clus + 2;
}

// 该簇号在FAT表的位置，求得的是扇区数
static inline uint32 fat_sec_of_clus(uint32 cluster, uint8 fat_num)
{
    return fat.bpb.rsvd_sec_cnt + fat.bpb.hidd_sec + (cluster << 2) / fat.bpb.byts_per_sec + fat.bpb.fat_sz * (fat_num - 1);
}

// 簇在扇区内的偏移
static inline uint32 fat_offset_of_clus(uint32 cluster)
{
    return (cluster << 2) % fat.bpb.byts_per_sec;
}

static inline uint32 next_cluster(uint32 cluster)
{
    char *fat = kmalloc(PAGE_SIZE);
    disk_read(fat, fat_sec_of_clus(cluster, 1), 1);
    uint32 offset = fat_offset_of_clus(cluster);
    uint32_t next = *(uint32_t*)(fat + offset);
    kfree(fat);
    return next;
}

static inline uint32 get_cluster_from_dentry(dentry_t *p)
{
    return ((uint32_t)p->high_start_clus << 16) + p->low_start_clus;
}

static inline uint32_t clus_of_sec(uint32_t sector)
{
    return (sector - fat.first_data_sec) / fat.bpb.sec_per_clus + 2;
}

static inline void set_dentry_cluster(dentry_t *p, uint32 cluster)
{
    p->high_start_clus = (cluster >> 16) & ((1lu << 16) - 1);
    p->low_start_clus = cluster & ((1lu << 16) - 1);
}

// 0: not empty, 1: empty
extern uint8 dentry_is_empty(dentry_t *p);
extern dentry_t* next_entry(dentry_t *p, char* buff, uint32_t* now_clus, uint32_t* now_sector);
extern char unicode2char(uint16_t unich);
extern uint16_t char2unicode(char ch);
extern dentry_t* search(const char *name, uint32_t dir_first_clus, char *buf, type_t mode, struct dir_pos *pos);
extern uint8_t filenamecmp(const char *name1, const char *name2);
extern inode_t find_dir(inode_t cur_cwd, const char *name);
extern dentry_t* create_new(char *name, uint32_t cluser_num, char *buf, type_t type, struct dir_pos *pos);
extern dentry_t* find_entry_entry(uint32_t dir_first_clus, char *buf, uint32_t num, uint32_t *sec);
extern uint32_t find_entry_clus(char *buf);
dentry_t *search2(const char *name, uint32_t dir_first_clus, char *buf, type_t mode, struct dir_pos *pos);
int set_fd(void *pcb, uint i, dentry_t *p, uint32_t flags);

static inline void set_cluster_for_dentry(dentry_t *p, uint32_t clus)
{
    // return ((uint32_t)p->high_start_clus << 16) + p->low_start_clus;
    p->high_start_clus = (uint16_t)((clus >> 16)& 0xffffu);
    p->low_start_clus = (uint16_t)(clus & 0xffffu);
    printk("new clus: %d\n", ((uint32_t)p->high_start_clus << 16) + p->low_start_clus);
}

static inline uint32 get_length_from_dentry(dentry_t *p)
{
    return p->file_size;
}

extern fat_t fat;
extern inode_t cwd, root;
extern char cwd_path[FAT32_MAX_PATH];

int fat32_init();
long fat32_getcwd(char *buf, size_t size);
int fat32_chdir(char *dest_path);
int fat32_mkdirat(fd_num_t dirfd, const char *path_const, uint32_t mode);
int fat32_getdents(uint32_t fd, char *outbuf, uint32_t len);
int fat32_mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data);
int fat32_umount(const char *special, int flags);
int fat32_fstat(int fd, struct stat *statfs);
int fat32_pipe2(uint32_t *fd);
int fat32_dup(int src_fd);
int fat32_dup3(int src_fd, int dst_fd);
int fat32_link();
int fat32_unlink(int dirfd, const char* path, uint32_t flags);

/* fat32 open close */
int fat32_openat(fd_num_t fd, const char *path, uint32 flags, uint32 mode);
int64 fat32_close(fd_num_t fd);

/* fat32 read write */
int64 fat32_read(fd_num_t fd, char *buf, size_t count);
int64 fat32_write(fd_num_t fd,const char *buf, uint64_t count);
int64 fat32_read_uncached(fd_num_t fd, char *buf, size_t count);
/* lseek */
int64 fat32_lseek(fd_num_t fd, size_t off, uint32_t whence);
uint64_t fat32_mmap(void *start, size_t len, int prot, int flags, int fd, off_t off);
int64 fat32_munmap(void *start, size_t len);

// final competition
#define MSDOS_SUPER_MAGIC  0x4d44 //虚拟机里面测出的sd卡类型

typedef struct {int val[2];} fsid_t;
struct statfs {
    long f_type; /* 文件系统类型 */
    long f_bsize; /* 经过优化的传输块大小 */
    long f_blocks; /* 文件系统数据块总数 */
    long f_bfree; /* 可用块数 */
    long f_bavail; /* 非超级用户可获取的块数 */
    long f_files; /* 文件结点总数 */
    long f_ffree; /* 可用文件结点数 */
    fsid_t f_fsid; /* 文件系统标识 */
    long f_namelen; /* 文件名的最大长度 */
};

int64_t fat32_fcntl(fd_num_t fd, int32_t cmd, uint64_t arg);
int64 fat32_readv(fd_num_t fd, struct iovec *iov, int iovcnt);
int64 fat32_writev(fd_num_t fd, struct iovec *iov, int iovcnt);
ssize_t fat32_pread(int fd, void * buf, size_t count, off_t offset);
ssize_t fat32_pwrite(int fd, void * buf, size_t count, off_t offset);
int32_t fat32_utimensat(fd_num_t dirfd, const char *pathname, const struct timespec times[2], int32_t flags);
int fat32_fstatat(fd_num_t dirfd, const char *pathname, struct stat *statbuf, int32 flags);
int fat32_statfs(const char *path, struct statfs *buf);
int fat32_fstatfs(int fd, struct statfs *buf);
void *fat32_mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address);

int32_t fat32_faccessat(fd_num_t dirfd, const char *pathname, int mode, int flags);
int32_t fat32_fsync(fd_num_t fd);
size_t fat32_readlinkat(fd_num_t dirfd, const char *pathname, char *buf, size_t bufsiz);
size_t fat32_sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
int fat32_msync(void *addr, size_t length, int flags);
int fat32_renameat2(fd_num_t olddirfd, const char *oldpath_const, fd_num_t newdirfd, 
                    const char *newpath_const, unsigned int flags);
                   
#endif
