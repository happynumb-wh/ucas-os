#ifndef FUTEX_H
#define FUTEX_H

#include <os/list.h>
#include <os/lock.h>
#include <os/time.h>
#include <type.h>
#include <os/errno.h>

#define FUTEX_BUCKETS 100
#define MAX_FUTEX_NUM 60

#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_FD		2
#define FUTEX_REQUEUE		3
#define FUTEX_CMP_REQUEUE	4
#define FUTEX_WAKE_OP		5
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8
#define FUTEX_WAIT_BITSET	9
#define FUTEX_WAKE_BITSET	10
#define FUTEX_WAIT_REQUEUE_PI	11
#define FUTEX_CMP_REQUEUE_PI	12

#define FUTEX_PRIVATE_FLAG	128
#define FUTEX_CLOCK_REALTIME	256
#define FUTEX_CMD_MASK		~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

#define FUTEX_WAIT_PRIVATE	(FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE	(FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#define FUTEX_REQUEUE_PRIVATE	(FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PRIVATE (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_OP_PRIVATE	(FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)
#define FUTEX_LOCK_PI_PRIVATE	(FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_UNLOCK_PI_PRIVATE	(FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_TRYLOCK_PI_PRIVATE (FUTEX_TRYLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_BITSET_PRIVATE	(FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_BITSET_PRIVATE	(FUTEX_WAKE_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_REQUEUE_PI_PRIVATE	(FUTEX_WAIT_REQUEUE_PI | \
					 FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PI_PRIVATE	(FUTEX_CMP_REQUEUE_PI | \
					 FUTEX_PRIVATE_FLAG)


typedef uint64_t futex_key_t;

typedef struct futex_node
{
    futex_key_t futex_key;
    list_node_t list;
    list_head block_queue;
	struct timespec add_ts;
	struct timespec set_ts;
}futex_node_t;

struct robust_list {
	struct robust_list *next;
};
struct robust_list_head {
	/*
	 * The head of the list. Points back to itself if empty:
	 */
	struct robust_list list;

	/*
	 * This relative offset is set by user-space, it gives the kernel
	 * the relative position of the futex field to examine. This way
	 * we keep userspace flexible, to freely shape its data-structure,
	 * without hardcoding any particular offset into the kernel:
	 */
	long futex_offset;

	/*
	 * The death of the thread may race with userspace setting
	 * up a lock's links. So to handle this race, userspace first
	 * sets this field to the address of the to-be-taken lock,
	 * then does the lock acquire, and then adds itself to the
	 * list, and then clears this field. Hence the kernel will
	 * always have full knowledge of all locks that the thread
	 * _might_ have taken. We check the owner TID in any case,
	 * so only truly owned locks will be handled.
	 */
	struct robust_list *list_op_pending;
};

typedef list_head futex_bucket_t;
// static spinlock_init(futex_lock);
extern struct spinlock futex_lock;

extern futex_bucket_t futex_buckets[FUTEX_BUCKETS]; //HASH LIST
extern futex_node_t futex_node[MAX_FUTEX_NUM];
extern int futex_node_used[MAX_FUTEX_NUM];
extern void check_futex_timeout();
extern void init_system_futex();
extern int do_futex_wait(int *val_addr, int val, const struct timespec *timeout);
extern int do_futex_wakeup(int *val_addr, int num_wakeup);
extern int do_futex_requeue(int *uaddr, int* uaddr2, int num);
extern int do_futex(uint32_t *uaddr, int futex_op, int val, const struct timespec *timeout,/* or: uint32_t val2 */int *uaddr2, int val3);
extern long do_get_robust_list(int pid, struct robust_list_head **head_ptr, size_t *len_ptr);
extern long do_set_robust_list(struct robust_list_head *head, size_t len);
#endif /* FUTEX_H */
