/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdlib.h>

#undef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

typedef struct list_head {
    struct list_head *next, *prev;
} LIST_HEAD;

#define LIST_HEAD_INIT(name) { &(name), &(name) }
static inline void INIT_LIST_HEAD(LIST_HEAD *entry)
{
    entry->next = entry->prev = entry;
}

static inline void __list_insert(LIST_HEAD *entry, LIST_HEAD *prev,
    LIST_HEAD *next)
{
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

static inline void LIST_INSERT_TAIL(LIST_HEAD *entry, LIST_HEAD *head)
{
    __list_insert(entry, head->prev, head);
}

#define LIST_FIRST(ptr, type, member) \
    container_of((ptr)->next, type, member)

#define LIST_FOREACH(pos, head, member) \
    for (pos = LIST_FIRST((head), typeof(*(pos)), member); \
        (&pos->member != (head)); \
        pos = container_of((pos)->member.next, typeof(*(pos)), member))

#endif /* __QUEUE_H__ */
