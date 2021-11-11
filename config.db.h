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
 */

#ifndef __CONFIG_DB_H__
#define __CONFIG_DB_H__

#include "config.parser.h"
#include "llist.h"

#define SUCCESS 0

typedef struct menu {
	_string_t prompt;
	_expr_t dependancy;

	/* ... list of items in the menu. */
	struct list_head entries;
	struct list_head childs, sibling;
} menu_t;

/* ... parser output. */
extern menu_t mainmenu;
extern _token_list_t config_files;

struct entry {
	_string_t prompt;	/* ... entry's prompt string. */
	_string_t symbol;	/* ... entry's configuration symbol. */
 	_expr_t dependancy; /* ... dependancy tree for this symbol. */
 	_string_t help;		/* ... help statements. */
 };

static inline _string_t dupprompt(_string_t s) {
	size_t pp;
	_string_t tmp = NULL;

	if ((pp = strlen(s)) > 2) {
		/* ... not an empty quotation. */

		int i;
		for (i = 0; i < pp &&
			s[i] != '\n'; i++); /* ... check for '\n'. */
		if (i == pp && (tmp = malloc(pp)) != NULL) {
			sscanf(s, "\"%[^\"]\"", tmp);
			free(s);
		}
	}

	return tmp;
}

static inline int __init_entry(struct entry *entry,
	token_t prompt,	token_t symbol, token_t help, _expr_t expr) {

	if (prompt.TK_STRING != NULL) {

		/* ... prompt is not a 'NULLDESC'. */ 
		entry->prompt = dupprompt(prompt.TK_STRING);
		if (entry->prompt == NULL)
			return -1;
	} else
		entry->prompt = NULL;

	entry->symbol = symbol.TK_STRING;
	entry->dependancy = expr;
	entry->help = help.TK_STRING;

	return SUCCESS;
}

#define TK_LIST_EF_NULL 0
#define TK_LIST_EF_DEFAULT 1
#define TK_LIST_EF_CONFIG 2
#define TK_LIST_EF_SELECTED 4

struct extra_token {
	unsigned long flags;
	token_t token;
};

typedef struct extra_token * _extra_token_t;
#define ETOKEN_SIZE sizeof(struct extra_token)

/* ... 'next_token' is called for every item's token. */
extern _token_list_t next_token(_token_list_t, int, ...);

typedef struct item {
	struct entry common;

/* ... records times an item refrenced. */
#define item_inc(_i) (_i)->refcount++
#define item_dec(_i) (_i)->refcount--
	unsigned long refcount;

#define item_token_list(_i) (_i)->__list
#define item_config_token(_i) 			\
	/* ... assuming a config item. */	\
	token_list_entry_info(item_token_list(_i)) 
 	_token_list_t __list;

 	struct list_head list;
 	struct hlist_node hnode;
} item_t;

#define SYMTABLE 256
extern void init_symbol_hash_table(void);
extern int __populate_config_file(const char *, int);

static inline int create_config_file(const char *filename) {
	return __populate_config_file(filename,
		/* ... create new file with defaults. */
		TK_LIST_EF_DEFAULT);
}

static inline int write_config_file(const char *filename) {
	return __populate_config_file(filename,
		TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED);
}

extern int read_config_file(const char *);
extern void __fprintf_menu(FILE *, menu_t *);

static inline int build_autoconfig(const char *filename) {
	FILE *fp;

	if ((fp = fopen(filename, "w+")) == NULL)
		return -1;
	__fprintf_menu(fp, &mainmenu);
	fclose(fp);

	return SUCCESS;
}

extern int push_menu(token_t, _expr_t);
extern int add_new_config_entry(token_t, token_t,
	token_t, _token_list_t, _expr_t, token_t);
extern int add_new_choice_entry(token_t, token_t,
	_token_list_t, _expr_t, token_t);
extern int add_new_config_file(token_t);
extern int pop_menu(void);

extern _expr_t expr_op_symbol_one(enum expr_op, token_t);
extern _expr_t expr_op_symbol_two(enum expr_op, token_t, token_t);
extern _expr_t expr_op_expr_one(enum expr_op, _expr_t);
extern _expr_t expr_op_expr_two(enum expr_op, _expr_t, _expr_t);
extern bool eval_expr(_expr_t);

extern void __toggle_choice(_extra_token_t, _string_t);
static inline void toggle_choice(item_t *item, _string_t n) {
	_token_list_t tp;

	token_list_for_each_entry(tp, item->__list) {
		_extra_token_t etoken = token_list_entry_info(tp);

		/* ... remove 'TK_LIST_EF_SELECTED', first. */
		etoken->flags &= ~TK_LIST_EF_SELECTED;
		__toggle_choice(etoken, n);
	}
}

extern void toggle_config(item_t *, ...);
extern int start_gui(int);

#endif /* __CONFIG_DB_H__ */