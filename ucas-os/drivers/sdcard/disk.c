#include "type.h"
#include "param.h"
//#include "memlayout.h"
#include "riscv.h"
#include "buf.h"
#include "sdcard.h"
#include "virtio.h"
#include <pgtable.h>
#include <os/sched.h>

void disk_init(void)
{
    #ifdef k210
        sdcard_init();
    #endif
}

uint8_t disk_read(uint8_t *data_buff, uint32_t sector, uint32_t count)
{
        // printk("disk read begin\n");
        for (int i = 0; i < count; i++)
        {
            // printk("disk_read: 0x%lx,%d,%d\n",data_buff, sector, i);
            // struct buf *b = bread(0,sector + i);
            // memcpy(data_buff + 512 * i,b->data,512);
            // brelse(b);
            /* code */
            sd_read_sector(data_buff + 512 * i, sector + i, 1);
        }
        // printk("disk read over\n");
        return 0;
        // return 0xFF;
        // return sd_read_sector(data_buff, sector, count);
}

uint8_t disk_write(uint8_t *data_buff, uint32_t sector, uint32_t count)
{
        // printk("disk write begin\n");
        for (int i = 0; i < count; i++)
        {
            /* code */
            // struct buf *b = bget(0,sector + i);
            // memcpy(b->data,data_buff + 512 * i,512);
            // b->valid = 1;
            // b->dirty = 1;
            // brelse(b);
            // bwrite(b);
            sd_write_sector(data_buff + 512 * i, sector + i, 1);
        }
        // printk("disk write end\n");
        return 0;
        // return 0xFF;
}
uint8_t disk_read_bio(struct buf *a, uint32_t num)
{
    // printk("start read bio for sector %d\n",a->sectorno);
    uint8_t r = sd_read_sector_bio(a, num);
    // printk("read bio done!\n");
    return r;
}
uint8_t disk_write_bio(struct buf *a, uint32_t num)
{
    // printk("start write bio\n");
    uint8_t r = sd_write_sector_bio(a, num);
    // printk("write bio done!\n");
    return r;
}
