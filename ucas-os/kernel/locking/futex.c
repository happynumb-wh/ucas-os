#include <os/futex.h>
#include <os/irq.h>
#include <os/mm.h>
#include <assert.h>
#include <os/sched.h>

futex_bucket_t futex_buckets[FUTEX_BUCKETS]; //HASH LIST
futex_node_t futex_node[MAX_FUTEX_NUM];
int futex_node_used[MAX_FUTEX_NUM] = {0};
struct robust_list_head *robust_list[NUM_MAX_TASK] = {0};

void init_system_futex()
{
    // acquire(&futex_lock);
    for (int i = 0; i < FUTEX_BUCKETS; ++i) {
        init_list(&futex_buckets[i]);
    }
    // release(&futex_lock);
}

static int futex_hash(uint64_t x)
{
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ul;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebul;
    x = x ^ (x >> 31);
    return x % FUTEX_BUCKETS;
}

static futex_node_t* get_node(int *val_addr, int create)
{
    int key = futex_hash((uint64_t)val_addr);
    list_node_t *head = &futex_buckets[key];
    for (list_node_t *p = head->next; p != head; p = p->next) {
        futex_node_t *node = list_entry(p, futex_node_t, list);
        if (node->futex_key == (uint64_t)val_addr) {
            // printk("[get_node] find futex_node, val_addr = %ld\n",val_addr);
            return node;
        }
    }
    if (create) {
        // futex_node_t *node = (futex_node_t*) kmalloc(sizeof(futex_node_t));
        int i = 0;
        for ( i = 0; i < MAX_FUTEX_NUM; i++){
          if(futex_node_used[i] == 0)
             break; 
        }
        if(i == MAX_FUTEX_NUM){
            // printk("[get_node] error: futex_node array is full\n");
            assert(0);
        }
        futex_node_used[i] = current_running->pid;
        futex_node_t *node = &futex_node[i];
        node->futex_key = (uint64_t)val_addr;
        init_list(&node->block_queue);
        list_add_tail(&node->list, &futex_buckets[key]);
        // printk("[get_node] alloc futex_node %d (val_addr = %d)\n",i,val_addr);
        node->set_ts.tv_nsec = 0;
        node->set_ts.tv_sec = 0;
        node->add_ts.tv_nsec = 0;
        node->add_ts.tv_sec = 0;
        return node;
    }

    return NULL;
}

int do_futex_wait(int *val_addr, int val, struct timespec *timeout)
{
    // acquire(&futex_lock);
    futex_node_t *node = get_node(val_addr,1);
    assert(node != NULL);
    // printk("timeout :%d, %d\n", timeout->tv_sec, timeout->tv_nsec);
    if(timeout && (timeout->tv_nsec != 0 || timeout->tv_sec != 0)){
        node->set_ts.tv_sec = timeout->tv_sec;
        node->set_ts.tv_nsec = timeout->tv_nsec;
        do_gettimeofday(&node->add_ts);
    }else if(timeout == 0){
        node->set_ts.tv_sec = 0;
        node->set_ts.tv_nsec = 0;
    }
    // printk("0x%x block node: %lx.\n",val_addr, node);
    // release(&futex_lock);
    do_block(&current_running->list, &node->block_queue);
    // acquire(&futex_lock);
    // printk("0x%x unblock node: %lx.\n",val_addr, node);
    return 0;
    // release(&futex_lock);
}

int do_futex_wakeup(int *val_addr, int num_wakeup)
{
    // acquire(&futex_lock);
    futex_node_t *node = get_node(val_addr, 0);
    if (node != NULL) {
        for (int i = 0; i < num_wakeup; ++i) {
            if (list_empty(&node->block_queue)) 
                break;
            do_unblock(node->block_queue.next);
        }
    }
    return 0;
    // do_scheduler();
    // release(&futex_lock);
}
int do_futex_requeue(int *uaddr, int* uaddr2, int num){
    futex_node_t *node1 = get_node(uaddr, 0);
    futex_node_t *node2 = get_node(uaddr2, 0);
    // printk("node1: %lx, node2: %lx\n", node1, node2);
    if (node1 == NULL){
        node1 = get_node(uaddr, 1);
    }
    if (node2 == NULL){
        node2 = get_node(uaddr2, 1);
    }
    // printk("node1: %lx, node2: %lx\n", node1, node2);
    for (int i = 0; i < num; ++i) {
        if (list_empty(&node1->block_queue)) 
            break;
        list_head *pcb_node = node1->block_queue.next;
        list_del(pcb_node);
        list_add(pcb_node, &node2->block_queue); 
    }
}

//TODO
void check_futex_timeout() {
    struct timespec cur_time;
    do_gettimeofday(&cur_time);
    // printk("cur_time :%d, %d\n", cur_time.tv_sec, cur_time.tv_nsec);
    for (int i=0;i<MAX_FUTEX_NUM;i++){   
        if (futex_node_used[i]) {
            if(futex_node[i].set_ts.tv_nsec == 0 && futex_node[i].set_ts.tv_sec == 0){
                while (!list_empty(&futex_node[i].block_queue)){
                    do_unblock(futex_node[i].block_queue.next);
                        // printk("empty\n");
                }
            }
            // printk("set ts %d %d, add ts %d %d\n", futex_node[i].set_ts.tv_sec, futex_node[i].set_ts.tv_nsec, futex_node[i].add_ts.tv_sec, futex_node[i].add_ts.tv_nsec);
            if((cur_time.tv_sec - futex_node[i].add_ts.tv_sec)>futex_node[i].set_ts.tv_sec){
                futex_node[i].set_ts.tv_sec = 0;
                futex_node[i].set_ts.tv_nsec = 0;
                futex_node[i].add_ts.tv_sec = 0;
                futex_node[i].add_ts.tv_nsec = 0;
                while (1) {
                    if (list_empty(&futex_node[i].block_queue)){
                        // printk("empty\n");
                        break;
                    }
                    do_unblock(futex_node[i].block_queue.next);
                }
            }else if((cur_time.tv_sec - futex_node[i].add_ts.tv_sec) == futex_node[i].set_ts.tv_sec){
                if(futex_node[i].set_ts.tv_nsec && (cur_time.tv_nsec - futex_node[i].add_ts.tv_nsec)>=futex_node[i].set_ts.tv_nsec){
                    futex_node[i].set_ts.tv_sec = 0;
                    futex_node[i].set_ts.tv_nsec = 0;
                    futex_node[i].add_ts.tv_sec = 0;
                    futex_node[i].add_ts.tv_nsec = 0;
                    while (1) {
                        if (list_empty(&futex_node[i].block_queue)){
                            // printk("empty\n");
                            break;
                        }
                        do_unblock(futex_node[i].block_queue.next);
                    }
                }
            }
            // printk("[futex] futex num %d timeout!\n",i);   
   
        }
    }
}
int do_futex(int *uaddr, int futex_op, int val,
          	const struct timespec *timeout,   /* or: uint32_t val2 */
         	 int *uaddr2, int val3){
    // printk("[futex] op: %d, uaddr: 0x%x(%d),uaddr2: 0x%x, val: %d, timeout = 0x%x(%d)\n", futex_op, uaddr, *uaddr,uaddr2, val, timeout, timeout);
    if((futex_op & FUTEX_WAKE) == FUTEX_WAKE){
        // printk("futex waking, uaddr: %x\n", uaddr);
        do_futex_wakeup(uaddr, val);
    }else if((futex_op & FUTEX_WAIT) == FUTEX_WAIT){  // always 1
        if(*uaddr == val){
            // printk("futex waiting, uaddr: %x\n", uaddr);
            do_futex_wait(uaddr, val, timeout);
        }else{
            return -EAGAIN;
        }
    }
    if((futex_op & FUTEX_REQUEUE) == FUTEX_REQUEUE){
        // printk("futex requeue, uaddr: %x, uaddr2: %x\n", uaddr, uaddr2);
        do_futex_requeue(uaddr, uaddr2, timeout);
    }
    return 0;
}

int find_index(int pid){
    for (int i = 1; i < NUM_MAX_TASK; i++){
        if(pid == pcb[i].pid){
            return i;
        }
    }
    return -1;
}

long do_get_robust_list(int pid, struct robust_list_head **head_ptr, size_t *len_ptr){
   
    if(pid == 0){
        uint64_t cpu_id = get_current_cpu_id();
        pcb_t * current_running = cpu_id == 0 ? current_running_master : current_running_slave; 
        int current_index = find_index(current_running->pid);
        *head_ptr = robust_list[current_index];
        // printk("get robust list:%x\n", robust_list[current_index]);
        return 0;
    }else{
        int index = find_index(pid);
        if(index == -1){
            printk("get robust list, pid: %d is not found!\n", pid);
            return -ESRCH;
        }
        if(robust_list[index] == 0){
            printk("get robust list is empty!\n");
            return -EFAULT;
        }
        *head_ptr = robust_list[index];
        return 0;
    }
}


long do_set_robust_list(struct robust_list_head *head, size_t len){
    // printk("set:head:%x, len: %d\n", head, len);
    if(len != sizeof(struct robust_list_head)){
        printk("set robust list len is error!\n");
        return -EINVAL;
    }
    uint64_t cpu_id = get_current_cpu_id();
    pcb_t * current_running = cpu_id == 0 ? current_running_master : current_running_slave; 
    int current_index = find_index(current_running->pid);
    robust_list[current_index] = head;
    return 0;
}
