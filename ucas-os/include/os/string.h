
#ifndef INCLUDE_STRING_H_
#define INCLUDE_STRING_H_
#include <type.h>
extern void memcpy(void *dest, const void* src, size_t len);
extern int  memcmp(const void *ptr1, const void *ptr2, size_t num);
extern void memset(void *dest, uint8_t val, size_t len);
extern void memmove(void *dest, const void *src, size_t len);
extern void bzero(void *dest,  size_t len);
extern int strcmp(const char *str1, const char *str2);
extern int strncmp(const char *str1, const char *str2, int n);
extern char *strcpy(char *dest, const char *src);
extern char *strcat(char *dest, const char *src);
extern int strlen(const char *src);
extern int atoi(char *s, uint32_t mode);
extern long int atol (const char * str);

#endif
