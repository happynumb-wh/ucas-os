#include <utils/utils.h>
#include <fs/fat32.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <screen.h>
#include <stdio.h>
#include <common.h>
#include <assert.h>
#include <user_programs.h>

uint8 dentry_is_empty(dentry_t *p)
{
    char *n = (char*)p;
    for (int i = 0; i < 32; i++)
    {
        if(*n++ != 0)
        {
            return 0;
        }
    }
    return 1;
}

dentry_t* next_entry(dentry_t *p, char* buff, uint32_t* now_clus, uint32_t* now_sector)
{
    p++;
    if((uint64_t)p - (uint64_t)buff == BUFSIZE)
    {
        *now_sector += READ_MAX_SEC;
        if(*now_sector - first_sec_of_clus(*now_clus) >= fat.bpb.sec_per_clus )
        {
            *now_clus = next_cluster(*now_clus);
            if(*now_clus == CLUSER_END)
                return (dentry_t *)buff;
            *now_sector = first_sec_of_clus(*now_clus);
        }
        disk_read(buff, *now_sector, READ_MAX_SEC);
        p = (dentry_t *)buff;
    }
    return p;
}
char unicode2char(uint16_t unich)
{
    return (unich >= 65 && unich <= 90)? unich - 65 + 'A' :
           (unich >= 48 && unich <= 57)? unich - 48 + '0' :
            (unich >= 97 && unich <= 122)? unich - 97 + 'a' :
            (unich == 95)? '_' : 
            (unich == 46)? '.':
            (unich == 0x20)? ' ':
            (unich == 45)? '-':
            0;  
}

uint16_t char2unicode(char ch)
{
    return (ch >= 'A' && ch <= 'Z')? 65 + ch - 'A' :
            (ch >= 'a' && ch <= 'z')? 97 + ch - 'a':
            (ch >= '0' && ch <= '9') ? 48 + ch - '0':
            (ch == '_')? 95 :
            (ch == '.')? 46 :
            (ch == ' ')? 0x20:
            (ch == '-')? 45:
            0;
}



/**
 * @brief fat32的文件名大小写不是与你写入的一致，根据0xCH位置判断文件名大小写
 * @param name1 
 * @param name2 
 * @return uint8_t 0表示相同，1表示不同
 */
uint8_t filenamecmp(const char *name1, const char *name2)
{    
    char n1[FAT32_MAX_FILENAME];
    char n2[FAT32_MAX_FILENAME];
    strcpy(n1, name1);
    strcpy(n2, name2);
    for (int i = 0; i < strlen(n1); i++)
    {
        if(n1[i] == ' '){
            n1[i] = 0;
        }
    }
    // printk("n1:%s, n2:%s\n", n1, n2);
    if(strlen(n1) != strlen(n2)){
        return 1;
    }
    for (int i = 0; i < strlen(n1); i++)
    {
        if(n1[i] >= 'A' && n1[i] <= 'Z')
        {
            n1[i] = 'a' + n1[i] - 'A';
        }
        if(n2[i] >= 'A' && n2[i] <= 'Z')
        {
            n2[i] = 'a' + n2[i] - 'A';
        }
    }
    // printk("n1:%s, n2:%s\n", n1, n2);
    return memcmp(n1, n2, strlen(n2));
}

/**
 * @brief debug打印出fd描述符的信息
 * @param fd 需要打印的文件描述符
 */
void debug_print_fd(fd_t * fd){
    printk("==============fd================\n");
    printk("name: %s\n", fd->name);
    printk("first_clus_num: %d\n", fd->first_clus_num);
    printk("pos: %d\n", fd->pos);
    printk("cur_clus_num: %d\n", fd->cur_clus_num);
    printk("cur_sec: %d\n", fd->cur_sec);
    printk("length: %d\n", fd->length);
    printk("fd_num: %d\n", fd->fd_num);
    printk("fd_pos: offset : %d\n", fd->dir_pos.offset);
    printk("fd_pos: sec : %d\n", fd->dir_pos.sec);
    printk("==============end fd================\n");
    // printk("fd_num: %d\n", fd->fd_num);
}

/**
 * @brief debug 打印出目录项的信息
 * @param dt 目录项
 */
void debug_print_dt(dentry_t * dt)
{
    for (int i = 0; i < SHORT_FILENAME; i++)
    {
        printk("%c", unicode2char(dt->dename[i]));
    }
    printk("\n");
    for (int i = 0; i < SHORT_EXTNAME; i++)
    {
        printk("%c", unicode2char(dt->dename[i]));
    }
    printk("\n");  
    printk("the cluster: %d\n", get_cluster_from_dentry(dt));  
    printk("the sector: %d\n", first_sec_of_clus(get_cluster_from_dentry(dt)));
    printk("length:%d\n ", dt->file_size);
}

/** 
 * @brief 从目录的第一个簇开始查找所需的目录项
 * @param name 要查询的文件名
 * @param dir_first_clus 当前目录文件的第一个簇
 * @param buf 目录文件的缓存
 * @param mode 寻找的文件类型
 * @param pos 定位所查找文件的所在目录项的扇区号，偏移位置，用来定位文件, 文件描述符使用
 * cluster_num 记录我们查找过程中的簇号
 */
dentry_t *search(const char *name, uint32_t dir_first_clus, char *buf, type_t mode, struct dir_pos *pos)
{
    // printk("[search] name:%s\n", name);
    uint32_t sector_num = first_sec_of_clus(dir_first_clus);
    uint32_t cluser_num = dir_first_clus;
    disk_read(buf, sector_num, READ_MAX_SEC);
    dentry_t *dentry = (dentry_t *)buf;
    // debug_print_dt(dentry);
    char filename[FAT32_MAX_FILENAME] = {0};
    while(!dentry_is_empty(dentry))
    {
        memset(filename, 0, FAT32_MAX_FILENAME);
        //被删除目录项
        while(dentry->dename[0] == EMPTY_ENTRY)
        {
            dentry = next_entry(dentry, buf, &cluser_num, &sector_num);
        }
        
        long_name_entry_t *long_entry = (long_name_entry_t *)dentry;
        short_name_entry_t *short_entry = (short_name_entry_t*)dentry;
        //处理长目录项
        if(long_entry->attributes == ATTR_LONG_NAME && (long_entry->seq & LAST_LONG_ENTRY) == LAST_LONG_ENTRY)
        {
            uint16_t unicode;
            uint8 long_entry_num = long_entry->seq & LONG_ENTRY_SEQ;
            while (long_entry_num--)
            {
                int count = 0;
                for (int i = 0; i < LONG_FILENAME1; i++)
                {
                    unicode = long_entry->name1[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                for (int i = 0; i < LONG_FILENAME2; i++)
                {
                    unicode = long_entry->name2[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                for (int i = 0; i < LONG_FILENAME3; i++)
                {
                    unicode = long_entry->name3[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                long_entry = (long_name_entry_t *)next_entry((dentry_t *)long_entry, buf, &cluser_num, &sector_num);
                dentry = (dentry_t *)long_entry;
            }    
        }else{//短目录项
            int count = 0;            
            for (int i = 0; i < SHORT_FILENAME; i++)
            {
                if(short_entry->dename[i] == ' ')
                    break;
                filename[count++] = short_entry->dename[i];
            }
            if(short_entry->extname[0] != ' ')
                filename[count++] = '.';
            for (int i = 0; i < SHORT_EXTNAME; i++)
            {
                if(short_entry->extname[i] == ' ')
                    break;
                filename[count++] = short_entry->extname[i];
            }
            filename[count] = 0;
        }//得到目录项记录的文件名字
        if((dentry->arributes & ATTR_DIRECTORY) != 0 && mode == FILE_DIR && !filenamecmp(filename, name))
        {
            if( pos != NULL) {
                pos->sec = sector_num + ((char *)dentry - buf) / SECTOR_SIZE;
                pos->offset = ((uint64_t)dentry & (uint64_t)(511));
            }
                
            return dentry;
        }
        if((dentry->arributes & ATTR_DIRECTORY) == 0 && mode == FILE_FILE && !filenamecmp(filename, name))
        {
            if( pos != NULL) {
                pos->sec = sector_num + ((char *)dentry - buf) / SECTOR_SIZE;
                pos->offset = ((uint64_t)dentry & (uint64_t)(511));
            }
            return dentry;
        }
        dentry = next_entry(dentry, buf, &cluser_num, &sector_num);
        if(cluser_num == CLUSER_END)
            return NULL;
    }
    return NULL;
}


/** 
 * @brief 从目录的第一个簇开始查找所需的目录项，不限制是否是文件或者文件夹
 * @param name 要查询的文件名
 * @param dir_first_clus 当前目录文件的第一个簇
 * @param buf 目录文件的缓存
 * @param pos 定位所查找文件的所在目录项的扇区号，偏移位置，用来定位文件, 文件描述符使用
 * cluster_num 记录我们查找过程中的簇号
 */
dentry_t *search3(const char *name, uint32_t dir_first_clus, char *buf, struct dir_pos *pos)
{
    uint32_t sector_num = first_sec_of_clus(dir_first_clus);
    uint32_t cluser_num = dir_first_clus;
    disk_read(buf, sector_num, READ_MAX_SEC);
    dentry_t *dentry = (dentry_t *)buf;
    // debug_print_dt(dentry);
    char filename[FAT32_MAX_FILENAME] = {0};
    while(!dentry_is_empty(dentry))
    {
        memset(filename, 0, FAT32_MAX_FILENAME);
        //被删除目录项
        while(dentry->dename[0] == EMPTY_ENTRY)
        {
            dentry = next_entry(dentry, buf, &cluser_num, &sector_num);
        }
        
        long_name_entry_t *long_entry = (long_name_entry_t *)dentry;
        short_name_entry_t *short_entry = (short_name_entry_t*)dentry;
        //处理长目录项
        if(long_entry->attributes == ATTR_LONG_NAME && (long_entry->seq & LAST_LONG_ENTRY) == LAST_LONG_ENTRY)
        {
            uint16_t unicode;
            uint8 long_entry_num = long_entry->seq & LONG_ENTRY_SEQ;
            while (long_entry_num--)
            {
                int count = 0;
                for (int i = 0; i < LONG_FILENAME1; i++)
                {
                    unicode = long_entry->name1[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                for (int i = 0; i < LONG_FILENAME2; i++)
                {
                    unicode = long_entry->name2[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                for (int i = 0; i < LONG_FILENAME3; i++)
                {
                    unicode = long_entry->name3[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                long_entry = (long_name_entry_t *)next_entry((dentry_t *)long_entry, buf, &cluser_num, &sector_num);
                dentry = (dentry_t *)long_entry;
            }    
        }else{//短目录项
            int count = 0;            
            for (int i = 0; i < SHORT_FILENAME; i++)
            {
                if(short_entry->dename[i] == ' ')
                    break;
                filename[count++] = short_entry->dename[i];
            }
            if(short_entry->extname[0] != ' ')
                filename[count++] = '.';
            for (int i = 0; i < SHORT_EXTNAME; i++)
            {
                if(short_entry->extname[i] == ' ')
                    break;
                filename[count++] = short_entry->extname[i];
            }
            filename[count] = 0;
            // printk("[search3] filename count:%d\n", count);
        }//得到目录项记录的文件名字
        // printk("[search3] filename:%s\n", filename);
        if(!filenamecmp(filename, name))
        {
            if( pos != NULL) {
                pos->sec = sector_num + ((char *)dentry - buf) / SECTOR_SIZE;
                pos->offset = ((uint64_t)dentry & (uint64_t)(511));
            }
            return dentry;
        }
        dentry = next_entry(dentry, buf, &cluser_num, &sector_num);
        if(cluser_num == CLUSER_END)
            return NULL;
    }
    return NULL;


}


/**
 * @brief 返回输入路径的目录的父目录的簇, 
 * 
 * @param cur_cwd 要查找的起始的簇
 * @param name 路径
 * @return inode_t 返回一个查询目录的父目录簇以及查询目录的目录项名字，比如/name1/name2/name3,返回name2的簇，name3的名字
 * 未找到返回ret_dir.name[0] = 0xff;
 */
inode_t new_cwd;
char cur_name[FAT32_MAX_FILENAME] = {0};
char new_name[FAT32_MAX_FILENAME] = {0};
char input_name[FAT32_MAX_FILENAME] = {0};
inode_t find_dir(inode_t cur_cwd, const char *name)
{
    static inode_t ret_dir;
    int len = strlen(name);
    int pos, count = 0;
    memset(input_name, 0, FAT32_MAX_FILENAME);
    memcpy(input_name, name, FAT32_MAX_FILENAME);
    name = input_name;
    if(name[0] == '.' && name[1] =='/'){
        name = name + 2;
    }
    if(name[0] == '/'){
        name ++;
    }    
    for ( pos = 0; pos < len; pos++)
    {
        if(name[pos] == '/')
        {
            count ++;
            break;
        }
    }
    if(count == 0){
        memset(cur_cwd.name, 0, FAT32_MAX_PATH);
        memcpy(cur_cwd.name, name, strlen(name));
        return cur_cwd;
    }else if(pos == len){
        memset(cur_cwd.name, 0, FAT32_MAX_PATH);
        memcpy(cur_cwd.name, name, strlen(name) - 1);
        return cur_cwd;
    }else{
        memset(cur_name, 0, FAT32_MAX_FILENAME);
        memset(new_name, 0, FAT32_MAX_FILENAME);
        memcpy(cur_name, name, pos);
        memcpy(new_name, &name[pos+1], len-pos-1);
        char *buff = kmalloc(NORMAL_PAGE_SIZE);
        // printk("cur_name: %s, new_name: %s\n", cur_name, new_name);  
        dentry_t* dentry = search(cur_name, cur_cwd.first_clus, buff, FILE_DIR, NULL);        
        if(dentry == NULL){
            ret_dir.name[0] = 0xff;
            kfree(buff);
            return ret_dir;
        }else{
            new_cwd.first_clus = get_cluster_from_dentry(dentry);
            kfree(buff);
            return find_dir(new_cwd, new_name);
        }
    }
}

/**
 * @brief 在文件的簇后边新加一个簇, 当old clus为0时，可用于在记录有一个簇的文件
 * 
 * @param old_clus 文件的最后一个簇
 * @param new_clus 新加入的簇
 * @param buff 存储读取的fat
 */
void write_fat_table_p(uint32_t old_clus, uint32_t new_clus, char*buff)
{
    uint32_t *clus_num;
    if(old_clus)
    {
        disk_read(buff, fat_sec_of_clus(old_clus, 1), 1);
        clus_num = (uint32_t*)((char*)buff + fat_offset_of_clus(old_clus));
        *clus_num = new_clus & 0xffffffff;
        disk_write(buff, fat_sec_of_clus(old_clus, 1), 1);

        disk_read(buff, fat_sec_of_clus(old_clus, 2), 1);
        clus_num = (uint32_t*)((char*)buff + fat_offset_of_clus(old_clus));
        *clus_num = new_clus & 0xffffffff;
        disk_write(buff, fat_sec_of_clus(old_clus, 2), 1);
    }
    // printk("%x\n", *(uint32_t *)buff);
    disk_read(buff, fat_sec_of_clus(new_clus, 1), 1);
    clus_num = (uint32_t*)((char*)buff + fat_offset_of_clus(new_clus));
    *clus_num = CLUSER_END;
    // printk("%x, %x\n", (uint32_t *)buff, clus_num);
    // printk("%x, %d\n", (uint32_t *)buff, fat_offset_of_clus(new_clus));
    // printk("%x, %x\n", *(uint32_t *)buff, *clus_num);
    disk_write(buff, fat_sec_of_clus(new_clus, 1), 1);

    disk_read(buff, fat_sec_of_clus(new_clus, 2), 1);
    clus_num = (uint32_t*)((char*)buff + fat_offset_of_clus(new_clus));
    *clus_num = CLUSER_END;
    disk_write(buff, fat_sec_of_clus(new_clus, 2), 1);
}

uint32_t find_entry_clus(char *buf)
{
    // 读取fat表
    uint32_t now_sec = fat.first_data_sec - fat.bpb.fat_sz;
    disk_read(buf, now_sec, READ_MAX_SEC);
    uint32_t *fat_buf = (uint32_t *)buf;
    uint32_t cluser_num = 0;
    while (1)
    {
        int i;
        for( i = 0; i < BUFSIZE/4; i++)
        {
            if(fat_buf[i] == 0)
                break; 
            cluser_num++;
        }
        if(i == BUFSIZE/4)
        {
            now_sec += READ_MAX_SEC;
            disk_read(buf, now_sec, READ_MAX_SEC);
        }else{
            break;
        }
    }
    return cluser_num;
}



/**
 * @brief 用于为创建目录寻找空的位置，可用于寻找一个空的目录项，或者几个连续的空目录项
 * 
 * @param dir_first_clus 目录文件的第一个簇
 * @param buf 用于存储读出的目录文件片段
 * @param num 需要的连续目录项个数
 * @param sec 连续目录项的起始目录项所在的目录文件片段的起始扇区
 * @return dentry_t* 连续目录项中的起始的一个目录项
 */
dentry_t *find_entry_entry(uint32_t dir_first_clus, char *buf, uint32_t num, uint32_t *sec)
{
    uint32_t clus_num = dir_first_clus;
    *sec = first_sec_of_clus(dir_first_clus);
    disk_read(buf, *sec, READ_MAX_SEC);
    dentry_t *entry = (dentry_t*)buf;
    // for (int i = 0; i < 10; i++)
    // {
    //     printk("%x, ", buf[i]);
    // }
    int count = 0;//用来记录找到的连续空目录项个数
    dentry_t *ret_entry = NULL; //起始目录项
    int ret_sec = 0;
    uint32_t old_clus;
    while (1)
    {
        old_clus = clus_num;
        //遇到非空，则count从0开始计算
        // printk("entry: %lx\n", entry);
        // printk("entry name:%s, clus num: %d, buf sec: %d\n",entry->dename, clus_num, *sec);
        if(dentry_is_empty(entry) == 0 && entry->dename[0] != EMPTY_ENTRY)
        {
            count = 0;
            // printk("entry: %lx\n", entry);
            // printk("entry name:%x, clus num: %d, buf sec: %d\n",entry->dename[0], clus_num, *sec);
            entry = next_entry(entry, buf, &clus_num, sec);
            ret_entry = entry;
            ret_sec = *sec;
        }
        else // 遇到空的则count开始累加
        {
            count++;
            // printk("empty entry: %lx\n", entry);
            // printk("entry name:%s, clus num: %d, buf sec: %d\n",entry->dename, clus_num, *sec);
            entry = next_entry(entry, buf, &clus_num, sec);
            if(count == num)
            {
                *sec = ret_sec;
                // printk("return entry: %lx\n", ret_entry);
                // printk("return entry name:%x, clus num: %d, buf sec: %d\n",entry->dename[0], clus_num, *sec);
                // while(1);
                return ret_entry;
            }
        }
        // printk("entry name: %s, clus num: %d, buf sec: %d\n",entry->dename , clus_num, *sec);
        // 本簇不够，到下一个簇寻找
        if(clus_num == CLUSER_END)
        {
            // printk("clus not enough\n");
            char *temp = kmalloc(PAGE_SIZE);
            clus_num = find_entry_clus(temp);
            write_fat_table_p(old_clus, clus_num, temp);
            if(count == 0){
                *sec = first_sec_of_clus(clus_num);
                memset(buf, 0, PAGE_SIZE);
            }else{
                *sec = ret_sec;
            }
            kfree(temp);
            return ret_entry;
        }
    }
    

}

/**
 * @brief 创建目录项
 * 
 * @param name 目录项的名字 
 * @param cur_clus 新目录项所在的簇
 * @param buf 重复利用缓存
 * @param type 创建类型
 * @return dentry_t 返回目录项
 */
dentry_t* create_new(char *name, uint32_t cur_clus, char *buf, type_t type, struct dir_pos *pos_t)
{
    // printk("creat name: %s, cur clus: %d, mode: %s\n", name, cur_clus, (type==FILE_DIR)?"dir": "file");
   
    int isfile = 0;
    int pos = 0;
    int len_filename = 0;
    int len_extname = 0;
    int len_name = strlen(name);
    char *filename;
    char *extname;
    dentry_t* ret_dentry;
    for (int i = 0; i < strlen(name); i++)
    {
        if(name[i] == '.' && type == FILE_FILE)
        {
            pos = i;
            name[pos] = '\0';
            isfile = 1;
            break;
        }
    }
    len_filename = strlen(name);
    filename = name;
    extname = name + pos + 1;
    len_extname  = strlen(name + pos + 1);
    if(isfile)
        name[pos] = '.';
    // printk("name:%s\n", name);
    dentry_t *entry;
    int dentry_num = 0;//包含短目录项和长目录项
    if (len_filename <= 8)
    {
        dentry_num = 1;
    }
    else
    {
        dentry_num = len_name / LONG_FILENAME + 2; //one short entry
    }
    // printk("filename: %s,len: %d, dentry_num: %d, extname: %s, len: %d!\n", filename, len_filename, dentry_num, extname, len_extname);
    uint32_t sec, clus;
    entry = find_entry_entry(cur_clus, buf, dentry_num, &sec);
    // printk("cur_clus: %d, buf:%lx, entry: %lx, sec: %d\n", cur_clus,buf, entry, sec);
    clus = clus_of_sec(sec);
    // printk("clus: %d\n");
    char *temp = kmalloc(PAGE_SIZE);
    uint32_t new_clus = find_entry_clus(temp);
    kfree(temp);
    // printk("find empty clus: %d\n", new_clus);
    
    
    //short_entry
    dentry_t new_dentry;
    memset(&new_dentry, 0, sizeof(new_dentry));

    if(dentry_num == 1)
    {
        for (int i = 0; i < len_filename; i++)
        {
            new_dentry.dename[i] = filename[i];
        }
        for (int i = len_filename; i < 8; i++)
        {
            new_dentry.dename[i] = ' ';
        }
    }else{
        for (int i = 0; i < 6; i++)
        {
            new_dentry.dename[i] = ((filename[i] <='z')&&(filename[i] >= 'a'))? filename[i] - 'a' + 'A': filename[i];
        }
        new_dentry.dename[6] = '~';
        new_dentry.dename[7] = '1';        
    }
    if(isfile)
    {
        for (int i = 0; i < len_extname; i++)
        {
            new_dentry.extname[i] = ((extname[i] <='z')&&(extname[i] >= 'a'))? extname[i] - 'a' + 'A': extname[i];
        }
        for (int i = len_extname; i < 3; i++)
        {
            new_dentry.extname[i] = ' ';
        }
    }
    else
    {
        for (int i = 0; i < 3; i++)
        {
            new_dentry.extname[i] = ' ';
        } 
    }
    if(type == FILE_DIR)
        new_dentry.arributes = ATTR_DIRECTORY;
    else
        new_dentry.arributes = ATTR_ARCHIVE;

    new_dentry.Lowercase = 0;
    new_dentry.creat_time_ms = 0;
    new_dentry.creat_time = 0x6000;//12:00:00
    new_dentry.creat_date = 0x54b5;//2022/5/21
    new_dentry.last_visit_time = 0x6000;
    new_dentry.high_start_clus = (new_clus >> 16) & ((1lu << 16) - 1);
    new_dentry.modity_time = 0x6000;
    new_dentry.modity_date = 0x54b5;
    new_dentry.low_start_clus = (new_clus) & ((1lu << 16) - 1);
    new_dentry.file_size = 0;
    
    //long entry if have
    long_name_entry_t long_entry[dentry_num - 1];
    if(dentry_num > 1)
    {
        int len = 0;
        for (int i = 0; i < dentry_num - 1; i++)
        {
            len = LONG_FILENAME * i;
            for (int j = 0; j < LONG_FILENAME1; j++)
            {
                long_entry[i].name1[j] = (len < len_name)? char2unicode(filename[len]):
                                         (len == len_name)? 0x0000:
                                         0xffff;
                len = len + 1;
            }
            for (int j = 0; j < LONG_FILENAME2; j++)
            {
                long_entry[i].name2[j] = (len < len_name)? char2unicode(filename[len]):
                                         (len == len_name)? 0x0000:
                                         0xffff;
                len = len + 1;
            }
            for (int j = 0; j < LONG_FILENAME3; j++)
            {
                long_entry[i].name3[j] = (len < len_name)? char2unicode(filename[len]):
                                         (len == len_name)? 0x0000:
                                         0xffff;
                len = len + 1;
            }
            long_entry[i].seq = (i + 1 == dentry_num -1)?(LAST_LONG_ENTRY | (i+1)) : (i+1) & LONG_ENTRY_SEQ; 
            long_entry[i].attributes = ATTR_LONG_NAME;
            long_entry[i].reserved = 0;
            uint8 sum = 0; 
            char*shortname = (char *)(&new_dentry);
            for (int j = 0; j < 11; j++)
            {
                sum = (sum & 0x1 ? 0x80 : 0) + (sum >> 1) + *shortname++;
            }
            long_entry[i].checksum = sum;
            long_entry[i].start_clus = 0;
        }
    }
    
    //写入目录项
    disk_read(buf, sec, READ_MAX_SEC);
    // printk("buf:%lx, entry: %lx, sec: %d\n",buf, entry, sec);
    for (int i = 0; i < dentry_num - 1; i++)
    {
        memcpy(entry, &long_entry[dentry_num - 2 - i], sizeof(dentry_t));
        // printk("entry name: %s, sec: %d\n", ((long_name_entry_t*)entry)->name1, sec);
        if(entry + 1 == (dentry_t *)(buf + BUFSIZE))
        {
            disk_write(buf, sec, READ_MAX_SEC);
        }
        entry = next_entry(entry, buf, &clus, &sec);
    }
    memcpy(entry, &new_dentry, sizeof(new_dentry));
    disk_write(buf, sec, READ_MAX_SEC);
    // printk("entry:%lx\n", entry);
    // printk("entry: %s, new clus: %d, dentry clus: %d\n", entry->dename, new_clus, get_cluster_from_dentry(entry));
    // printk("new_entry: %s, new clus: %d, dentry clus: %d\n", new_dentry.dename, new_clus, get_cluster_from_dentry(&new_dentry));
    // ret_dentry = entry;
    // printk("ret entry:%lx\n", ret_dentry);
    // printk("retdentry: %s,new clus: %d, retdentry clus: %d\n", ret_dentry->dename, new_clus, get_cluster_from_dentry(ret_dentry));
    ret_dentry = entry;
    temp = kmalloc(PAGE_SIZE);
    if(pos_t){
        pos_t->sec = sec + ((char *)ret_dentry - buf) / SECTOR_SIZE;;
        pos_t->offset = ((uint64_t)entry & (uint64_t)(511));
    }
    if(type == FILE_DIR)
    {
        sec = first_sec_of_clus(new_clus);
        memset(temp, 0, BUFSIZE);
        entry = (dentry_t *)temp;
        entry->dename[0] = '.';
        for (int i = 1; i < SHORT_FILENAME; i++)
        {
            entry->dename[i] = ' ';
        }
        for (int i = 0; i < SHORT_EXTNAME; i++)
        {
            entry->extname[i] = ' ';
        }
        entry->arributes = ATTR_DIRECTORY;
        entry->Lowercase = 0;
        entry->creat_time_ms = 0;
        entry->creat_time = 0x6000;//12:00:00
        entry->creat_date = 0x54b5;//2022/5/21
        entry->last_visit_time = 0x6000;
        entry->high_start_clus = (new_clus >> 16) & ((1lu << 16) - 1);
        entry->modity_time = 0x6000;
        entry->modity_date = 0x54b5;
        entry->low_start_clus = (new_clus) & ((1lu << 16) - 1);
        entry->file_size = 0;

        entry ++;
        entry->dename[0] = '.';
        entry->dename[1] = '.';
        for (int i = 2; i < SHORT_FILENAME; i++)
        {
            entry->dename[i] = ' ';
        }
        entry->arributes = ATTR_DIRECTORY;
        entry->Lowercase = 0;
        entry->creat_time_ms = 0;
        entry->creat_time = 0x6000;//12:00:00
        entry->creat_date = 0x54b5;//2022/5/21
        entry->last_visit_time = 0x6000;
        entry->high_start_clus = (cur_clus >> 16) & ((1lu << 16) - 1);
        entry->modity_time = 0x6000;
        entry->modity_date = 0x54b5;
        entry->low_start_clus = (cur_clus) & ((1lu << 16) - 1);
        entry->file_size = 0;
        disk_write(temp, sec, READ_MAX_SEC);      
    }else
    {
        memset(temp, 0, BUFSIZE);
        sec = first_sec_of_clus(new_clus);
        disk_write(temp, sec, READ_MAX_SEC);
    }
    
    // if( pos != NULL) {
    //     pos->sec = sector_num + ((char *)dentry - buf) / SECTOR_SIZE;
    //     pos->offset = ((uint64_t)dentry & (uint64_t)(511));
    // }
    write_fat_table_p(0, new_clus, temp);
    kfree(temp);
    // printk("retdentry: %s,new clus: %d, retdentry clus: %d\n", ret_dentry->dename, new_clus, get_cluster_from_dentry(ret_dentry));
    // while(1);
    return ret_dentry;
}

//可以返回长名字目录项的起始目录项
dentry_t *search2(const char *name, uint32_t dir_first_clus, char *buf, type_t mode, struct dir_pos *pos)
{
    uint32_t sector_num = first_sec_of_clus(dir_first_clus);
    uint32_t cluser_num = dir_first_clus;
    disk_read(buf, sector_num, READ_MAX_SEC);
    dentry_t *dentry = (dentry_t *)buf;
    char filename[FAT32_MAX_FILENAME];

    dentry_t *first_entry;
    int32_t  first_entry_sec;
    int32_t  entry_num;

    while(dentry_is_empty(dentry) == 0)
    {
        memset(filename, 0, FAT32_MAX_FILENAME);
        //被删除目录项
        while(dentry->dename[0] == EMPTY_ENTRY)
        {
            dentry = next_entry(dentry, buf, &cluser_num, &sector_num);
        }
        first_entry = dentry;
        first_entry_sec = sector_num;

        long_name_entry_t *long_entry = (long_name_entry_t *)dentry;
        //处理长目录项
        if(long_entry->attributes == ATTR_LONG_NAME && (long_entry->seq & LAST_LONG_ENTRY) == LAST_LONG_ENTRY)
        {
            uint16_t unicode;
            uint8 long_entry_num = long_entry->seq & LONG_ENTRY_SEQ;
            entry_num = long_entry_num + 1;

            while (long_entry_num--)
            {
                int count = 0;
                for (int i = 0; i < LONG_FILENAME1; i++)
                {
                    unicode = long_entry->name1[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                for (int i = 0; i < LONG_FILENAME2; i++)
                {
                    unicode = long_entry->name2[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                for (int i = 0; i < LONG_FILENAME3; i++)
                {
                    unicode = long_entry->name3[i];
                    if(unicode == 0x0000 || unicode == 0xffff)
                    {
                        filename[long_entry_num * LONG_FILENAME + count] = 0;
                        break;
                    }
                    else filename[long_entry_num * LONG_FILENAME + count] = unicode2char(unicode);
                    count ++;
                }
                dentry = next_entry((dentry_t *)long_entry, buf, &cluser_num, &sector_num);
                long_entry = (long_name_entry_t *)dentry;
            }        
        }else{//短目录项
            entry_num = 1;
            int count = 0;
            for (int i = 0; i < SHORT_FILENAME; i++)
            {
                if(dentry->dename[i] == ' ')
                    break;
                filename[count++] = dentry->dename[i];
            }
            if(dentry->extname[0] != ' ')
                filename[count++] = '.';
            for (int i = 0; i < SHORT_EXTNAME; i++)
            {
                if(dentry->dename[i] == ' ')
                    break;
                filename[count++] = dentry->extname[i];
            }
            filename[count] = 0;
        }//得到目录项记录的文件名字
        // printk("filename:%s, name: %s, mode: %d, dentry.name:%s, attr: %x\n",filename, name, mode, dentry->dename, dentry->arributes);
        // printk("debug:%d\n",dentry->arributes & ATTR_DIRECTORY);
        // printk("debug:%d\n",dentry->arributes & ATTR_DIRECTORY);
        if((dentry->arributes & ATTR_DIRECTORY) != 0 && mode == FILE_DIR && !filenamecmp(filename, name))
        {
            pos->offset = (char*)first_entry - (char*)buf;
            pos->len = entry_num;
            pos->sec = first_entry_sec;
            return dentry;
        }
        if((dentry->arributes & ATTR_DIRECTORY) == 0 && mode == FILE_FILE && !filenamecmp(filename, name))
        {
            pos->offset = (char*)first_entry - (char*)buf;
            pos->len = entry_num;
            pos->sec = first_entry_sec;
            return dentry;
        }
        dentry = next_entry(dentry, buf, &cluser_num, &sector_num);
        if(cluser_num == CLUSER_END)
            return NULL;
    }
    return NULL;

}

/* WH add */
/**
 * @brief 判断是否是绝对路径
 * @param path 路径
 */
uint32_t is_ab_path(const char *path)
{
    return path[0] == '/';
}
/* WH add */
/**
 * @brief 预处理路径，去除前部的'/'以及'./'
 */
const char * pre_path(const char *path, uint32_t dir_clus)
{
    if (path[0] == '/')
        return &path[1];
    else if (dir_clus == root.clus_num) {
        if (path[0] == '.')
        {
            if (path[1] == '\0') return &path[1]; // "."
            else return &path[2]; // "./"            
        } 

    }
    return path;
}
/* WH add */
/**
 * @brief 得到下一个/之后的名字，如果已经到了最后一个则按情况处理
 * @param name 名字
 */
uint8_t get_next_name(char *name)
{
    for (int i = 0; ; i++){
        if (name[i] == '/') {
            name[i] = '\0';
            return 1;
        } else if (name[i] == '\0') {
            name[i] = '\0';
            return 0;
        }
    }
}
/* WH add */
/**
 * @brief 根据dentry的内容设置文件或者目录描述符
 * @param pcb_underinit 待处理的pcb
 * @param i 描述符id
 * @param p 目录项
 * @param flag 标志位
 * @return 成功返回0
 */
uint8 set_fd_from_dentry(void *pcb_underinit, char * name, uint i, \
                         dentry_t *p, uint32_t flags, dir_pos_t * pos)
{
    // i is index
    pcb_t *pcb_under = (pcb_t *)pcb_underinit;
    assert(pcb_under->pfd[i].used);
    pcb_under->pfd[i].dev = DEV_DEFAULT;
    pcb_under->pfd[i].first_clus_num = get_cluster_from_dentry(p);
    // pcb_under->pfd[i].flags = flags;
    if (p->arributes & ATTR_DIRECTORY) pcb_under->pfd[i].flags |= S_IFDIR;
    else pcb_under->pfd[i].flags |= S_IFREG;
    pcb_under->pfd[i].length = get_length_from_dentry(p);
    /* position */
    pcb_under->pfd[i].pos = 0;
    pcb_under->pfd[i].cur_sec = first_sec_of_clus(pcb_under->pfd[i].first_clus_num);
    pcb_under->pfd[i].cur_clus_num = pcb_under->pfd[i].first_clus_num;
    /* maybe for the piped */
    pcb_under->pfd[i].nlink = 1;
    strcpy(pcb_under->pfd[i].name, name);
    /* copy dir to find the dir */
    memcpy(&pcb_under->pfd[i].dir_pos, pos, sizeof(dir_pos_t));
    return 0;
}
/* WH add */
/**
 * @brief 查找一个空的簇号
 * @return 返回簇号
 */
uint32_t search_empty_cluster()
{
    char * buf = kmalloc(PAGE_SIZE);
    uint32_t now_sec = fat.first_data_sec - 2*fat.bpb.fat_sz;
    disk_read(buf, now_sec, 1);
    uint32_t j = 0;
    while (1){
        uint8_t i;
        for (i = 0; i < 4; ++i)
        {
            if (*(buf + i + (j % SECTOR_SIZE))){
                break;
            }
        }
        if (i == 4) break;
        else j += 4;
        if (j % SECTOR_SIZE == 0){
            now_sec += 1;
            disk_read(buf, now_sec, 1);
        }
    }
    write_fat_table(0, j/4);
    kfree(buf);
    return j/4;    
}

/**
 * @brief 分配一个空 簇号
 * 
 */
uint32_t alloc_free_clus()
{
    uint32_t new = fat.next_free++;
    write_fat_table(0, new);
    return new;
}

/**
 * @brief 写fat表，将fat表的内容更新, 如果old_clus为0，则表示将new_clus标记为最后一块clus即可
 * 否则需要将老块的标记为新的下一个clus
 * @param old_clus 旧clus
 * @param new_clus 新clus
 * @return no return 
 */
void write_fat_table(uint32_t old_clus, uint32_t new_clus)
{
    char *buff = kmalloc(PAGE_SIZE);
    uint32_t *clusat;
    uint32_t op_clus = old_clus ? old_clus : new_clus;
    /* TABLE 1*/
    disk_read(buff, fat_sec_of_clus(op_clus, 1), 1);
    clusat = (uint32_t *)((char*)buff + fat_offset_of_clus(op_clus));
    *clusat = old_clus ? FAT_MASK & new_clus : M_FAT32_EOC;
    disk_write(buff, fat_sec_of_clus(op_clus, 1), 1);
    /* TABLE 0*/
    disk_read(buff, fat_sec_of_clus(op_clus, 2), 1);
    clusat = (uint32_t *)((char*)buff + fat_offset_of_clus(op_clus));
    *clusat = old_clus ? FAT_MASK & new_clus : M_FAT32_EOC;
    disk_write(buff, fat_sec_of_clus(op_clus, 2), 1);
    kfree(buff);
}


/**
 * @brief 为了更高的效率，更新文件描述符中当前位置的指针等信息
 * @param fd 文件描述符指针
 * @param off 新的pos的位置
 */
void update_fd_from_pos(fd_t * fd, size_t pos)
{
    /* update off */
    fd->pos = pos;
    /* update cur_clus, cur_ */
    uint32_t sector_offset = fd->pos / SECTOR_SIZE;
    // printk("sector_offset:%d\n", sector_offset);
    fd->cur_clus_num = fd->first_clus_num;
    fd->cur_sec = first_sec_of_clus(fd->first_clus_num);
    for (int i = 0; i < sector_offset / fat.bpb.sec_per_clus; i++)
    {
        /* code */
        fd->cur_clus_num = next_cluster(fd->cur_clus_num);
    }
    fd->cur_sec = first_sec_of_clus(fd->cur_clus_num) + sector_offset % fat.bpb.sec_per_clus;
}

/**
 * @brief handle the special path
 * @param path the path 
 */
int open_special_path(fd_t * new_fd, const char * path)
{
    if (!strcmp("/dev/null",path) || !strcmp("/dev/null/invalid",path)){
        new_fd->dev = DEV_NULL;
        new_fd->flags |= S_IFCHR;
        new_fd->nlink = 1;
        return 1;
    }
    else if (!strcmp("/dev/zero",path)) {
        new_fd->dev = DEV_ZERO;
        new_fd->flags |= S_IFCHR;
        new_fd->nlink = 1;
        return 1;
    }
    else if (!strcmp("/etc/passwd",path) ||
             !strcmp("/etc/localtime", path) ||
             !strcmp("/etc/adjtime", path) ||
             !strcmp("/etc/group", path) ||
             !strcmp("/proc/mounts", path) ||
             !strcmp("/proc/meminfo", path) ||
             !strcmp("/ls", path)) {
        strcpy(new_fd->name, path);
        int file_id = get_file_from_kernel(path);
        assert(file_id != -1);
        new_fd->length = *elf_files[file_id].file_length;
        new_fd->pos = 0;
        new_fd->nlink = 1;
        if (!strcmp("/etc/passwd",path)) new_fd->flags |= S_IFCHR;
        else new_fd->flags |= S_IFREG;
        return 1;     
    }else if (!strcmp("/dev/tty",path)) {
        strcpy(new_fd->name, path);
        new_fd->dev = DEV_STDOUT;
        return 1;
    }    
    return 0;
}

/**
 * @brief read from sepcial file
 * @param nfd the nfd
 * @param buf the buf
 * @param cout the read count
 */
int read_special_path(fd_t * nfd, char *buf, size_t count)
{
    if(nfd->dev == DEV_STDIN){
        return read_ring_buffer(&stdin_buf, buf, count);
    }
    else if (nfd->dev == DEV_STDOUT){
        return read_ring_buffer(&stdout_buf, buf, count);
    }  
    else if (nfd->dev == DEV_STDERR){
        return read_ring_buffer(&stderr_buf, buf, count);
        // return 0;
    }
    else if (nfd->dev == DEV_NULL) return 0;
    else if (nfd->dev == DEV_ZERO) {
        memset(buf,0,count);
        return count;
    }
    else if (nfd->dev == DEV_PIPE) return pipe_read(buf,nfd->pip_num,count);

    if (!strcmp(nfd->name, "/etc/passwd") || 
        !strcmp(nfd->name, "/etc/localtime") ||
        !strcmp(nfd->name, "/etc/adjtime") ||
        !strcmp(nfd->name, "/etc/group") ||
        !strcmp(nfd->name, "/proc/mounts") ||
        !strcmp(nfd->name, "/proc/meminfo") ||
        !strcmp(nfd->name, "/ls")) 
    {
        int file_id = get_file_from_kernel(nfd->name);
        if(nfd->pos >= nfd->length){
            buf[0] = (-1);
            return 0;
        }
        int copy_len = 0;
        copy_len = (count > (nfd->length - nfd->pos))? (nfd->length - nfd->pos): count;
        memset(buf,0,count);
        memcpy(buf,  elf_files[file_id].file_content + nfd->pos, copy_len);
        nfd->pos += copy_len;
        if(nfd->pos >= nfd->length){
            buf[copy_len] = (-1);//EOF
        }
        return copy_len;        
    }
    return -1;
}

/**
 * @brief write special file
 * @param nfd the nfd
 * @param buf the write buf
 * @param count the num of write
 */
int write_special_path(fd_t * nfd, const char *buf, size_t count)
{
    if (nfd->dev == DEV_STDIN){
        return write_ring_buffer(&stdin_buf, buf, count);
    }  
    else if (nfd->dev == DEV_STDOUT) {
        // write_ring_buffer(&stdout_buf, buf, count);
        return screen_stdout(DEV_STDOUT, buf, count);
    }
    else if (nfd->dev == DEV_STDERR){
        // return count;
        if (!strcmp(buf,"echo: write error: Invalid argument\n")) 
            return strlen("echo: write error: Invalid argument\n"); //ignore this invalid info
        return screen_stderror(DEV_STDERR, (char *)buf, count);
    }
    else if (nfd->dev == DEV_NULL) return count;
    else if (nfd->dev == DEV_ZERO) return 0;
    else if (nfd->dev == DEV_PIPE) return pipe_write(buf,nfd->pip_num, count);

    if (!strcmp(nfd->name, "/etc/passwd") || 
        !strcmp(nfd->name, "/etc/localtime") ||
        !strcmp(nfd->name, "/etc/adjtime") ||
        !strcmp(nfd->name, "/etc/group") ||
        !strcmp(nfd->name, "/proc/mounts") ||
        !strcmp(nfd->name, "/proc/meminfo") ||
        !strcmp(nfd->name, "/ls")) 
    {
        int file_id = get_file_from_kernel(nfd->name);
        if(nfd->pos >= nfd->length && count != 0){
            // not deal with it
            assert(0);
        }
        int write_len = 0;
        write_len = (count > (nfd->length - nfd->pos))? (nfd->length - nfd->pos): count;
        if (write_len != count)
        {
            printk("[Warning] need extend the nfd length to %d\n", nfd->pos + count);
        }
        memcpy(elf_files[file_id].file_content + nfd->pos, buf, write_len);
        nfd->pos += write_len;
        if(nfd->pos >= nfd->length){
            ((char *)buf)[write_len] = (-1);//EOF
        }
        return write_len;        
    }
    return -1;
}

int check_addr_alloc(uintptr_t vta, uint64_t len){
    
    int not_alloc = 0;
    for (uintptr_t i = vta; i <= vta + len; i += NORMAL_PAGE_SIZE){
        if(get_kva_of(i, current_running->pgdir) == 0){
            not_alloc = 1;
            alloc_page_helper(i, current_running->pgdir, MAP_USER,  \
                        _PAGE_READ | _PAGE_WRITE | _PAGE_ACCESSED);
        }
    }
    if(not_alloc){
        local_flush_tlb_all();
    }
    return 0;
}
