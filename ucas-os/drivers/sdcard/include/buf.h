#ifndef __BUF_H
#define __BUF_H

#include <os/sleeplock.h>
#include <os/list.h>
#define BSIZE 512
#define BNUM 	1600

#define wait_queue_init(r) {initlock(&(r)->lock);\
                            (r)->head.prev = &(r)->head;\
                            (r)->head.next = &(r)->head;\
                            }


struct wait_queue {
	struct spinlock lock;
	list_head head;
};


struct buf {
    uint32              dev;        // 块设备号
    uint32              sectorno;   // 扇区号
    uint32              refcnt;
    uint16              valid;      // 内容是否有效
    uint8               dirty;      // 是否脏
    uint8               disk;       // 是否正在进行 I/O
    list_node_t         list;       // 双向链表节点
    list_node_t         hash;       // 哈希链表节点
    sleeplock_t         lock;
    char               data[BSIZE];
};

void			binit(void);
struct buf*		bget(uint dev, uint sectorno);
struct buf*		bread(uint dev, uint sectorno);
int				breads(struct buf *bufs[], int nbuf);
void			bwrite(struct buf *b);
int				bwrites(struct buf *bufs[], int nbuf);
void			brelse(struct buf *b);
void			bput(struct buf *b);
void			bsync(void);
void			bprint();
#endif
