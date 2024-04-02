// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "type.h"
#include "stdio.h"
#include "os/list.h"
#include "include/param.h"
#include "include/buf.h"
#include "include/sdcard.h"
#include "include/riscv.h"
#include "include/sdcard.h"
//#include "include/debug.h"
//#include "sched/proc.h"
#include <os/sleeplock.h>

#if BNUM >= 2000
#define BCACHE_TABLE_SIZE 300
#elif BNUM >= 1000
#define BCACHE_TABLE_SIZE 150
#else
#define BCACHE_TABLE_SIZE 50
#endif

#define __hash_idx(idx) ((idx) % BCACHE_TABLE_SIZE)

/**
 * Dirty buffer write back no-block mechanism.
 * 
 * When we need to write buf to disk, we don't wait.
 * Just throw it to the disk driver. Every time when a
 * disk interrupt comes, the driver will start the next
 * write task automatically.
 * 
 * Note that when those dirty bufs are sending to disk,
 * we don't hold their sleeplock, which might block us.
 * No matter whether there is another process who holds
 * some of them and reads/writes the data, we just carry
 * the data to disk without any damage of the data.
 * But if someone is writing the buf, we might carry the
 * unfinish work to disk. However, this seems to be
 * harmless.
 * 
 * Besides, we don't worry the eviction, because all
 * dirty bufs are not in the lru list and evictions occur
 * only on lru list in bget().
 */


// This lock protects @lru_head, @bcache, nwait
// and some other buf fields (see buf.h).

// @lru_head can only access unused bufs,
// while @bcache can get any valid ones.
static list_node_t lru_head;
static list_node_t bcache[BCACHE_TABLE_SIZE];

// If we run out all the bufs, we have to wait.
// This is the number of waiting procs.
int nwait = 0;

// Bufs pool.
static struct buf *bufs;
static int actual_buf_num;
// wait queue
static sleeplist_t nwait_sleep_queue;
void 
binit(void)
{
	sleeplist_init(&nwait_sleep_queue);
	init_list(&lru_head);
	nwait = 0;
	actual_buf_num = ROUND(sizeof(struct buf) * BNUM,NORMAL_PAGE_SIZE) / sizeof(struct buf);
	printk("> [binit] actual_buf_num = %d\n",actual_buf_num);
	bufs = kmalloc(NORMAL_PAGE_SIZE);
	int actual_buf_remain = ROUND(sizeof(struct buf) * BNUM,NORMAL_PAGE_SIZE) - NORMAL_PAGE_SIZE;
	for (;actual_buf_remain!=0;actual_buf_remain-=NORMAL_PAGE_SIZE) kmalloc(NORMAL_PAGE_SIZE); //we assume that the memspace allocated here is coherent 
	for (int i = 0; i < BCACHE_TABLE_SIZE; i++)
		init_list(&bcache[i]);
	for (struct buf *b = bufs; b < bufs + actual_buf_num; b++) {
		memset(b,0,sizeof(struct buf));
		list_add_tail(&b->list, &lru_head);
		initsleeplock(&b->lock);
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
struct buf*
bget(uint dev, uint sectorno)
{
	struct buf *b;
	int idx = __hash_idx(sectorno);
	/**
	 * Is the block already cached? Look up in the hash list at first.
	 * No matter which list the target buf stays now, we can always get
	 * it from the hash list (as long as it is cached).
	 */	
rescan:
	for (list_node_t *l = bcache[idx].next; l != &bcache[idx]; l = l->next) {
		b = container_of(l, struct buf, hash);
		if (b->dev == dev && b->sectorno == sectorno) {
			if (l != bcache[idx].next) {		// not the first node
				list_del(l);		// move it to the head (also lru)
				list_add(l, &bcache[idx]);
			}
			if (b->refcnt++ == 0) {
				/**
				 * We are the first visitor.
				 * This indicates that this buf is in the @lru_head list.
				 * We need to move it out in case of being evicted.
				 * The last one who keep the buf will push it back.
				 */
				list_del(&b->list);
			}
			acquiresleep(&b->lock);
			return b;
		}
	}

	/**
	 * We are here due to cache miss.
	 * Just get one from the tail of @lru_head list.
	 * This may cause an eviction, but it is safe.
	 */
	if (!list_empty(&lru_head)) {
		b = container_of(lru_head.prev, struct buf, list);
		if (b->dirty) disk_write_bio(b,1);
		b->dirty = 0;
		b->dev = dev;
		b->sectorno = sectorno;
		b->refcnt = 1;
		if (b->hash.prev) {		// if we were in a hash list
			list_del(&b->hash);
		}
		list_add(&b->hash, &bcache[idx]);
		list_del(&b->list);
		acquiresleep(&b->lock);		// this should not block
		b->valid = 0;
		return b;
	}
	/**
	 * We are here because the @lru_head list is
	 * empty, we have to go to sleep and wait.
	 * When we wake up, we need to re-scan both hash
	 * and lru list, in case of some procs who need
	 * the identical buf and wake up before us,
	 * taking a buf then insert into the hash list.
	 */
	//disk_write_start();
	nwait++;
	// printk("\n"__INFO("bget")" sleep\n");
	sleep(&nwait_sleep_queue);
	nwait--;
	goto rescan;
}

// Return a locked buf with the contents of the indicated block.
struct buf* 
bread(uint dev, uint sectorno)
{
	struct buf *b;
	b = bget(dev, sectorno);
	if (!b->valid) {
		disk_read_bio(b,1);
		b->valid = 1;
	}
	return b;
}

// Write b's contents to disk.  Must be locked.
void 
bwrite(struct buf *b)
{
	/**
	 * This function doesn't behave the same as the old one
	 * any more. In our case, we only commit the buf to the
	 * disk driver and directly come back without waiting.
	 * Most importantly, we release the buf in this function
	 * instead of brelse(). For two reasons:
	 * 1. bread() and brelse() make a pair for read cases,
	 * while bread() (or bget() if we'll write cover the buf)
	 * and bwrite() make the other pair for write cases.
	 * 2. We need different operations.
	 */
	
	int res;
	/**
	 * Submit a write request to disk driver. If get positive
	 * result, it means the buf it new in driver's queue, so
	 * we need to keep a ref for the driver.
	 */
	res = disk_write_bio(b,1);
	b->dirty = 0;
	//b->valid = 0;
	brelse(b);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
	releasesleep(&b->lock);
	bput(b);
}

void bsync(void)
{
	list_node_t *b_list;
	for (b_list = lru_head.next;b_list!=&lru_head;b_list = b_list->next){
		struct buf *b = container_of(b_list, struct buf, list);
		if (b->dirty) disk_write_bio(b,1);
		b->dirty = 0;
	}
}


void bput(struct buf *b)
{
	b->refcnt--;
	if (b->refcnt == 0) {
		// no one is waiting for it.
		list_add(&b->list, &lru_head);

		// available buf for someone who's in sleep-wait
		if (nwait > 0)
			wakeup(&nwait_sleep_queue);
	}
}

void bprint()
{
	printk("\nbuf cache:\n");
	for (int i = 0; i < BCACHE_TABLE_SIZE; i++) {
		if (list_empty(&bcache[i])) { continue; }
		printk("%d", i);
		for (list_node_t *l = bcache[i].next; l != &bcache[i]; l = l->next) {
			struct buf *b = container_of(l, struct buf, hash);
			printk("%d ", b->sectorno);
		}
		printk("\n");
	}
}
