#ifndef	_INCLUDE_SOURCH_H
#define _INCLUDE_SOURCH_H
#include <os/list.h>
#include <type.h>

// source manager for list struct
typedef struct source_manager
{
    list_node_t free_source_list;
    list_node_t wait_list;
    uint64_t source_num; /* share page number */
}source_manager_t;

#endif