#ifndef ASSERT_H
#define ASSERT_H

#include <stdio.h>

static inline void _panic(const char* file_name,int lineno, const char* func_name)
{
    printk("Assertion failed at %s in %s:%d\n\r",
           func_name,file_name,lineno);
    for(;;);
}

static inline void _panic2(const char* file_name,int lineno, const char* func_name)
{
    printk("enter %s in %s:%d\n\r",
           func_name,file_name,lineno);
}

static inline void _panic3(const char* file_name,int lineno, const char* func_name)
{
    printk("leave %s in %s:%d\n\r",
           func_name,file_name,lineno);
}

#define assert(cond)                                 \
    {                                                \
        if (!(cond)) {                               \
            _panic(__FILE__, __LINE__,__FUNCTION__); \
        }                                            \
    }

#define debug(cond)                                  \
    {                                                \
        if (!(cond)) {                               \
            _panic2(__FILE__, __LINE__,__FUNCTION__);\
        } else                                       \
        {                                            \
            _panic3(__FILE__, __LINE__,__FUNCTION__);\
        }                                            \
    }


#endif /* ASSERT_H */
