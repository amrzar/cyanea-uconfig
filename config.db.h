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
#include "config.utils.h"

#define SUCCESS 0

typedef struct menu {
    _string_t prompt;
    _expr_t dependancy;

    /* ... list of items in the menu. */
    struct list_head entries;
    struct list_head childs, sibling;
} menu_t;

typedef struct config_file {
    _string_t file;
    menu_t *menu;       /* ... menu, the config file included in. */

    struct list_head node;
} *_config_file_t;

extern menu_t main_menu, *curr_menu;
extern struct list_head config_files;

struct entry {
    _string_t prompt;   /* ... entry's prompt string. */
    _string_t symbol;   /* ... entry's configuration symbol. */
    _expr_t dependancy; /* ... dependancy tree for this symbol. */
    _string_t help;     /* ... help statements. */
};

#define TK_LIST_EF_NULL 0
#define TK_LIST_EF_DEFAULT 1
#define TK_LIST_EF_CONFIG 2
#define TK_LIST_EF_SELECTED 4
#define TK_LIST_EF_CONDITIONAL 8

typedef struct extended_token {
    unsigned long flags;
    token_t token;

    struct token_list node;
} *_extended_token_t;

typedef struct item {
    struct entry common;

    unsigned long refcount;
#define item_inc(_i) (_i)->refcount++
#define item_dec(_i) (_i)->refcount--

    /* Token list stores list of extended tokens related to this item.
     *
     * Configuration Option - The first entry in this list has 'TK_LIST_EF_CONFIG'
     * flag and stores token for 'BOOL', 'INTEGER' or 'STRING' type. The remaining
     * entries are tokens from 'select' keyword with 'TK_LIST_EF_NULL' flag.
     *
     * Multiple Choices - List of tokens for 'option' keywords. The default entry
     * has 'TK_LIST_EF_DEFAULT' flag. */

    _token_list_t tk_list;
#define item_token_list_for_each(pos, _i) token_list_for_each(pos, (_i)->tk_list)

    struct list_head list;
    struct hlist_node hnode;
} item_t;

static inline _extended_token_t item_get_config_etoken(item_t *item) {
    _extended_token_t etoken = container_of(item->tk_list, struct extended_token, node);

    return etoken->flags & TK_LIST_EF_CONFIG ? etoken : NULL;
}

extern int __populate_config_file(const char *, unsigned long);
#define create_config_file(_f) __populate_config_file((_f), TK_LIST_EF_DEFAULT)
#define write_config_file(_f)  __populate_config_file((_f), \
    (TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED))

extern int read_config_file(const char *);

extern void __fprintf_menu(FILE *, menu_t *);
static inline int build_autoconfig(const char *filename) {
    FILE *fp;

    if ((fp = fopen(filename, "w+")) == NULL)
        return -1;

    fprintf(fp, "#ifndef __UCONFIG_H\n");
    fprintf(fp, "#define __UCONFIG_H\n");
    __fprintf_menu(fp, &main_menu);
    fprintf(fp, "#endif /* __UCONFIG_H */\n");
    fclose(fp);

    return SUCCESS;
}

extern void __toggle_choice(_extended_token_t, _string_t);
static inline void toggle_choice(item_t *item, _string_t n) {
    _token_list_t tp;

    item_token_list_for_each(tp, item) {
        _extended_token_t etoken = container_of(tp, struct extended_token, node);

        /* ... remove 'TK_LIST_EF_SELECTED', first. */
        etoken->flags &= ~TK_LIST_EF_SELECTED;
        __toggle_choice(etoken, n);
    }
}

extern void toggle_config(item_t *, ...);
extern bool eval_expr(_expr_t);

#endif /* __CONFIG_DB_H__ */