#ifndef SCREEN_DRIVER_H
#define SCREEN_DRIVER_H
#include <os/lock.h>
#include <type.h>
#define BUFFER_SIZE 4096

// the screen sruct for shell
#pragma pack(8)
typedef struct screen_buffer
{
    // mutex visit
    // mutex_lock_t screen_lock;
    // the buffer is full
    list_node_t wait_list;
    char buffer[BUFFER_SIZE];
    int32_t num;
    int32_t head;
    int32_t tail;
} screen_buffer_t;
#pragma()
// screen_buffer_t SCREEN;
// the buffer is full
int screen_buffer_full();

/* configuring screen properties */
void init_screen();

/* clear screen */
void screen_clear();

/* put a char */
void screen_write_ch(char ch);

/* reflush screen buffer */
void screen_reflush();

/* screen write string */
void screen_write(char *str);

/* tmp screen write */
int64 screen_stdout(int fd, char *buf, size_t count);

/* tmp screen write */
int64 screen_stderror(int fd, char *buf, size_t count);

extern struct ring_buffer stdin_buf;
extern struct ring_buffer stdout_buf;
extern struct ring_buffer stderr_buf;

#endif