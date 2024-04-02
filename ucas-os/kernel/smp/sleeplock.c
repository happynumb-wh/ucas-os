#include <os/sleeplock.h>
#include <os/list.h>
#include <assert.h>

// void 
// initsleeplist(sleeplist_t *sleep_queue)
// {
// 	init_list(&sleep_queue->sleep_queue);
// 	initlock(&sleep_queue->lk);
// }


void 
sleep(sleeplist_t *sleep_queue)
{
	
	// just sleep
	do_block(&current_running->list, &sleep_queue->sleep_queue);	
}

void 
wakeup(sleeplist_t *sleep_queue)
{
	// wake up all
	while (!list_empty(&sleep_queue->sleep_queue))
	{
		do_unblock(sleep_queue->sleep_queue.next);
	}
}


void
initsleeplock(struct sleeplock *lk)
{
	init_list(&lk->sleep_queue);
	lk->locked = 0;
	lk->pid = 0;	
}

void
acquiresleep(struct sleeplock *lk)
{
	
	while (lk->locked) {
		// printk("block %d", current_running->pid);
		// sleep(lk, &lk->lk);
		do_block(&current_running->list, &lk->sleep_queue);
	}
	lk->locked = 1;
	lk->pid = current_running->pid;
}

void
releasesleep(struct sleeplock *lk)
{
	lk->locked = 0;
	lk->pid = 0;
	
	while (!list_empty(&lk->sleep_queue))
		// wakee up all
		do_unblock(lk->sleep_queue.next);

}