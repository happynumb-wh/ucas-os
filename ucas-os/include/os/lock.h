/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Thread Lock
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_
#define MAX_LOCK_NUM 16
#define MAX_WAIT_QUEUE 16
#define MAX_BARRIER_NUM 16
#define MAX_MAILBOX_NUM 16
#define MAX_MSG_LENGTH 64

#include <os/list.h>
#include <os/spinlock.h>

#define USER_LOCK 0
#define USER_NULOCK 1

typedef spinlock_status_t lock_status_t;

typedef enum {
    USED,
    UNSED,
} user_lock_status_t;

typedef enum {
    MBOX_OPEN,
    MBOX_CLOSE,
} mailbx_ststus_t;

/* sleep lock */
typedef struct mutex_lock
{
    uint64_t pid;
    uint64_t status;
    list_head block_queue;
} mutex_lock_t;

#define mutex_init(lock) struct mutex_lock lock = \
                            {  .pid = 0,\
                               .status = UNLOCKED,\
                               .block_queue = {&(lock.block_queue), &(lock.block_queue)} \ 
                            }
                         

void spin_lock_init(spinlock_t *lock);
int spin_lock_try_acquire(spinlock_t *lock);
void spin_lock_acquire(spinlock_t *lock);
void spin_lock_release(spinlock_t *lock);

/* mutex lock */
void do_mutex_lock_init(mutex_lock_t *lock);
void do_mutex_lock_acquire(mutex_lock_t *lock);
void do_mutex_lock_release(mutex_lock_t *lock);

#endif
