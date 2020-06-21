/*
 * Copyright 2020 Amirreza Zarrabi <amrzar@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * THIS IS ORIGINALLY COPIED FROM LINUX KERNEL.
 *
 */

#ifndef __CONFIG_UTILS_H__
#define __CONFIG_UTILS_H__

#include <stdlib.h>

#undef offsetof
#define offsetof(type, member) ((size_t)&((type *)0)->member)

#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))

struct list_head {
	struct list_head *next, *prev; 
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define DEFINE_LIST(name) struct list_head name = LIST_HEAD_INIT(name)

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_for_each_entry(pos, head, member) \
	for (pos = list_entry((head)->next, typeof(*pos), member); \
	     &pos->member != (head); \
	     pos = list_entry(pos->member.next, typeof(*pos), member))

static inline int list_empty(const struct list_head *head) {
	return head->next == head;
}

static inline void init_list_head(struct list_head *entry) {
	entry->next = entry->prev = entry;
}

static inline void __list_add(struct list_head *new,
	struct list_head *prev, struct list_head *next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add_tail(struct list_head *_new,
	struct list_head *head) {
	__list_add(_new, head->prev, head);
}

struct hlist_head {
	struct hlist_node *first;
};
 
struct hlist_node {
	struct hlist_node *next, **pprev;
};

#define HLIST_HEAD_INIT { .first = NULL }
#define DEFINE_HLIST(name) struct hlist_head name = HLIST_HEAD_INIT

#define hlist_entry_safe(ptr, type, member) \
	({ typeof(ptr) _ptr = (ptr); \
		_ptr ? container_of(_ptr, type, member) : NULL; })

#define hlist_for_each_entry(pos, head, member) \
    for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member); \
		 pos; pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

static inline void init_hlist_head(struct hlist_head *head) {
	head->first = NULL;
}

static inline void init_hlist_node(struct hlist_node *entry) {
	entry->next = NULL;
	entry->pprev = NULL;
}

static inline void hlist_add_head(struct hlist_node *new,
	struct hlist_head *head) {
	struct hlist_node *first = head->first;

	new->next = first;
	if (first != NULL)
		first->pprev = &new->next;
	head->first = new;
	new->pprev = &head->first;
}

static inline int hlist_empty(struct hlist_head * head) {
	return head->first == NULL;
}

#endif /* __CONFIG_UTILS_H__ */