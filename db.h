/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DB_H__
#define __DB_H__

#include "config.parser.h"
#include "utils.h"

#define SUCCESS 0

typedef struct menu {
    string_t prompt;
    expr_t dependency;

    /* ... list of items in the menu. */
    struct list_head entries;
    struct list_head childs, sibling;
} menu_t;

struct config_file {
    string_t file;
    menu_t *menu;               /* ... menu, the config file included in. */

    struct list_head node;
};

extern menu_t main_menu, *curr_menu;
extern struct list_head config_files;

struct entry {
    string_t prompt;            /* ... entry's prompt string. */
    string_t symbol;            /* ... entry's configuration symbol. */
    expr_t dependency;          /* ... dependency tree for this symbol. */
    string_t help;              /* ... help statements. */
};

#define TK_LIST_EF_NULL 0
#define TK_LIST_EF_DEFAULT 1
#define TK_LIST_EF_CONFIG 2
#define TK_LIST_EF_SELECTED 4
#define TK_LIST_EF_CONDITIONAL 8

struct extended_token {
    unsigned long flags;
    token_t token;

    struct token_list node;
};

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
     * has 'TK_LIST_EF_DEFAULT' flag.
     *
     **/

    struct token_list *tk_list;

#define item_token_list_entry(ptr) \
    ({ typeof(ptr) ____ptr  = (ptr); \
        ____ptr ? container_of(____ptr , struct extended_token, node) : NULL; \
    })

#define item_token_list_for_each_entry(pos, i) \
    for (pos = item_token_list_entry((i)->tk_list); \
        pos != NULL; \
        pos = item_token_list_entry(pos->node.next))

    struct list_head list;
    struct hlist_node node;
} item_t;

static inline struct extended_token *item_get_config_etoken(item_t * item)
{
    struct extended_token *e = item_token_list_entry(item->tk_list);
    return e->flags & TK_LIST_EF_CONFIG ? e : NULL;
}

extern int __populate_config_file(const char *, unsigned long);
#define create_config_file(_f) __populate_config_file((_f), TK_LIST_EF_DEFAULT)
#define write_config_file(_f)  __populate_config_file((_f), \
    (TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED))

extern int read_config_file(const char *);

extern int fprintf_menu(FILE *, menu_t *);
static inline int build_autoconfig(const char *filename)
{
    FILE *fp;

    if ((fp = fopen(filename, "w+")) == NULL)
        return -1;

    fprintf(fp, "#ifndef __UCONFIG_H\n");
    fprintf(fp, "#define __UCONFIG_H\n");
    fprintf_menu(fp, &main_menu);
    fprintf(fp, "#endif /* __UCONFIG_H */\n");
    fclose(fp);

    return SUCCESS;
}

extern void __toggle_choice(struct extended_token *, string_t);
static inline void toggle_choice(item_t * item, string_t n)
{
    struct extended_token *etoken;

    item_token_list_for_each_entry(etoken, item) {
        /* ... remove 'TK_LIST_EF_SELECTED', first. */
        etoken->flags &= ~TK_LIST_EF_SELECTED;
        __toggle_choice(etoken, n);
    }
}

extern void toggle_config(item_t *, ...);
extern bool eval_expr(expr_t);

#endif /* __DB_H__ */
