#include <screen.h>
#include <os/string.h>
#include <common.h>
#include <sbi.h>
#include <os/sched.h>
#include <os/ring_buffer.h>

// struct screen_buffer SCREEN;
screen_buffer_t SCREEN;
char screen_buff[PAGE_SIZE];
int screen_buffer_full(){
    return SCREEN.num == BUFFER_SIZE;
}
struct ring_buffer stdin_buf;
struct ring_buffer stdout_buf;
struct ring_buffer stderr_buf;
// init buffer
void init_screen(){
    // init the mutex
    // do_mutex_lock_init(&SCREEN.screen_lock);

    init_list(&SCREEN.wait_list);
    // be zero
    SCREEN.head = SCREEN.tail = SCREEN.num = 0;
    memset((void *)SCREEN.buffer, 0, BUFFER_SIZE);
    init_ring_buffer(&stdin_buf);
    init_ring_buffer(&stdout_buf);
    init_ring_buffer(&stderr_buf);
}

// clear screen buffer 
void screen_clear(){
    //do_mutex_lock_acquire(&SCREEN.screen_lock);

    memset((void *)SCREEN.buffer, 0, BUFFER_SIZE);
    SCREEN.head = SCREEN.tail = SCREEN.num = 0; 
    //do_mutex_lock_release(&SCREEN.screen_lock);
}

// write one buffer to buffer
void screen_write_ch(char ch){

    SCREEN.buffer[SCREEN.tail++ % BUFFER_SIZE] = ch;
    SCREEN.tail %= BUFFER_SIZE;
}

// reflush screen buffer
void screen_reflush(){
    return;
    // do_mutex_lock_acquire(&SCREEN.screen_lock);
    // return 0;
    while(SCREEN.num > 0){
        SCREEN.num--;
        sbi_console_putchar(SCREEN.buffer[SCREEN.head++ % BUFFER_SIZE]);
    }
    
    SCREEN.head %= BUFFER_SIZE;
    while(!list_empty(&SCREEN.wait_list))
        do_unblock(SCREEN.wait_list.next);
    // do_mutex_lock_release(&SCREEN.screen_lock);
}

// screen write string
void screen_write(const char *str){
    // get length
    uint32_t length = strlen(str);
    // for (int i = 0; i < length; i++)
    // {
    //     printk("%c",str[i]);
    //     /* code */
    // }
    printk(str);
    return;
    while(length > BUFFER_SIZE - SCREEN.num)
    {
        do_block(&current_running->list, &SCREEN.wait_list);
    }

    SCREEN.num += length;
    for (int i = 0; i < length; i++)
    {
        screen_write_ch(str[i]);
    }
}

int64 screen_stdout(int fd, const char *buf, size_t count){
    if (count < PAGE_SIZE)
    {
        memcpy(screen_buff, buf, count);
        screen_buff[count] = '\0';
        printk(screen_buff);
    } else 
    {
        for (int i = 0; i < count; i++)
        {
            printk("%c",buf[i]);
            // printk(buf);
            /* code */
        }        
    }

    return count;
    
    //do_mutex_lock_acquire(&SCREEN.screen_lock);    
    // get length
    uint32_t length = count;
    while(length > BUFFER_SIZE - SCREEN.num)
    {
        do_block(&current_running->list, &SCREEN.wait_list);
    }
    SCREEN.num += length;
    for (int i = 0; i < length; i++)
    {
        screen_write_ch(buf[i]);
    }

    //do_mutex_lock_release(&SCREEN.screen_lock);
    /* return number */
    return count;
}

int64 screen_stderror(int fd, const char *buf, size_t count)
{
    for (int i = 0; i < count; i++)
    {
        printk("%c",buf[i]);
        /* code */
    }
    return count;
    //do_mutex_lock_acquire(&SCREEN.screen_lock);    
    // get length
    uint32_t length = count;
    while(length > BUFFER_SIZE - SCREEN.num)
    {
        do_block(&current_running->list, &SCREEN.wait_list);
    }
    SCREEN.num += length;
    for (int i = 0; i < length; i++)
    {
        screen_write_ch(buf[i]);
    }

    //do_mutex_lock_release(&SCREEN.screen_lock);
    /* return number */
    return count;   
}
