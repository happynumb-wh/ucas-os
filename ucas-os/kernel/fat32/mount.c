#include <fs/fat32.h>

uint32_t mount_clus;
char mountpoint[64];

int fat32_mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data)
{
    char *buf = kmalloc(PAGE_SIZE);
    char path[strlen(dir) + 1];
    strcpy(path, dir);
    inode_t cur_cwd;
    inode_t dir_ino;
    dentry_t *entry;
    struct dir_pos pos;
    if(path[0] == '/')
    {
        cur_cwd.first_clus = root.first_clus;
        dir_ino = find_dir(cur_cwd, path + 1);
    }
    else
    {
        cur_cwd.first_clus = cwd.first_clus;
        dir_ino = find_dir(cur_cwd, path);
    }
    if(dir_ino.name[0] == 0xff){
        printk("not find parent dir!\n");
        kfree(buf);
        return -ENOENT;
    }
    strcpy(mountpoint, dir);
    entry = search(dir_ino.name, dir_ino.first_clus, buf, FILE_DIR, &pos);
    mount_clus = get_cluster_from_dentry(entry);
    disk_read(buf, pos.sec, 1);
    set_dentry_cluster(entry, root.first_clus);
    disk_write(buf, pos.sec, 1);
    kfree(buf);
    return 0;
}

int fat32_umount(const char *special, int flags)
{
    char *buf = kmalloc(PAGE_SIZE);
    char path[strlen(mountpoint) + 1];
    strcpy(path, mountpoint);
    inode_t cur_cwd;
    inode_t dir_ino;
    dentry_t *entry;
    struct dir_pos pos;
    if(path[0] == '/')
    {
        cur_cwd.first_clus = root.first_clus;
        dir_ino = find_dir(cur_cwd, path + 1);
    }
    else
    {
        cur_cwd.first_clus = cwd.first_clus;
        dir_ino = find_dir(cur_cwd, path);
    }
    if(dir_ino.name[0] == 0xff)
        printk("not find!\n");
    entry = search(dir_ino.name, dir_ino.first_clus, buf, FILE_DIR, &pos);
    if(entry == NULL)
    {
        // printk("not find mountpoint!\n");
        kfree(buf);
        return -ENOENT;
    }
    disk_read(buf, pos.sec, 1);
    set_dentry_cluster(entry, mount_clus);
    disk_write(buf, pos.sec, 1);
    kfree(buf);
    return 0;
}