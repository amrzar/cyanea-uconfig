/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __CONFIG_UTILS_H__
#define __CONFIG_UTILS_H__

#include <stdlib.h>

#undef offsetof
#define offsetof(type, member) ((size_t)&((type *)0)->member)

#define container_of(ptr, type, member)                                 \
    ((type *)((char *)(ptr) - offsetof(type, member)))

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define list_for_each_entry(pos, head, member)                          \
    for (pos = container_of((head)->next, typeof(*(pos)), member);      \
        &pos->member != (head);                                         \
        pos = container_of((pos)->member.next, typeof(*(pos)), member))

#define list_add_tail(new, head)                                        \
    __list_add((new), (head)->prev, (head))

static inline void init_list_head(struct list_head *entry) {
    entry->next = entry->prev = entry;
}

static inline void __list_add(struct list_head *new,
    struct list_head *prev,
    struct list_head *next) {
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

struct hlist_head {
    struct hlist_node *first;
};

struct hlist_node {
    struct hlist_node *next, * *pprev;
};

#define HLIST_HEAD_INIT { .first = NULL }

#define hlist_entry_safe(ptr, type, member)                             \
    ({ typeof(ptr) _ptr = (ptr);                                        \
        _ptr ? container_of(_ptr, type, member) : NULL; })

#define hlist_for_each_entry(pos, head, member)                         \
    for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member); \
        pos; pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define init_hlist_head(head) do { (head)->first = NULL; } while(0)

#define init_hlist_node(entry) do {                                     \
        (entry)->next = NULL;                                           \
        (entry)->pprev = NULL;                                          \
    } while(0)

static inline void hlist_add_head(struct hlist_node *new,
    struct hlist_head *head) {
    struct hlist_node *first = head->first;

    new->next = first;

    if (first != NULL)
        first->pprev = &new->next;

    head->first = new;
    new->pprev = &head->first;
}

#define debug_print(...) fprintf(stderr, "[debug_print]" __VA_ARGS__)
#define error_print(...) fprintf(stderr, "[error_print]" __VA_ARGS__)

#endif /* __CONFIG_UTILS_H__ */