#ifndef __UTILS_H
#define __UTILS_H

#include <type.h>
#include <fs/fat32.h>
#include <fs/file.h>
#include <os/string.h>

//fat.c utils

// 0: not empty, 1: empty
uint8 dentry_is_empty(dentry_t *p);
dentry_t* next_entry(dentry_t *p, char* buff, uint32_t* now_clus, uint32_t* now_sector);
char unicode2char(uint16_t unich);
uint16_t char2unicode(char ch);
dentry_t *search(const char *name, uint32_t dir_first_clus, char *buf, type_t mode, struct dir_pos *pos);
uint8_t filenamecmp(const char *name1, const char *name2);
inode_t find_dir(inode_t cur_cwd, const char *name);
uint8 set_fd_from_dentry(void *pcb_underinit, char* name, uint i, dentry_t *p, uint32_t flags, dir_pos_t * pos);
uint32_t is_ab_path(const char *path);
uint8_t get_next_name(char *name);
const char * pre_path(const char *path, uint32_t dir_clus);
void debug_print_dt(dentry_t * dt);
void debug_print_fd(fd_t * fd);
uint32_t search_empty_cluster();
uint32_t alloc_free_clus();
void write_fat_table(uint32_t old_clus, uint32_t new_clus);
void update_fd_from_pos(fd_t * fd, size_t pos);
int open_special_path(fd_t * new_fd, const char * path);
int read_special_path(fd_t * nfd, char *buf, size_t count);
int write_special_path(fd_t * nfd, const char *buf, size_t count);
/* search3 */
dentry_t *search3(const char *name, uint32_t dir_first_clus, char *buf, struct dir_pos *pos);
/* check addr is alloc or not */ 
int check_addr_alloc(uintptr_t vta, uint64_t len);
#endif