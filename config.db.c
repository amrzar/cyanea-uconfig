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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>

#include "config.db.h"
#include "y.tab.h"

#define SYMTABLE 256

menu_t mainmenu = {
    NULL,
    NULL, /* ... always true. */
    LIST_HEAD_INIT(mainmenu.entries),
    LIST_HEAD_INIT(mainmenu.childs),
    LIST_HEAD_INIT(mainmenu.sibling)
};

static menu_t *curr_menu = MAINMENU;

_token_list_t config_files = NULL;
static struct hlist_head symtable[SYMTABLE];

void init_symbol_hash_table(void) {
    for (int i = 0; i < SYMTABLE; i++)
        init_hlist_head(&symtable[i]);
}

static unsigned long hash_symbol(_string_t symbol) {
    int c;
    unsigned long hash = 5381;

    while ((c = *symbol++) != '\0')
        hash = ((hash << 5) + hash) + c;

    return hash % SYMTABLE;
}

static item_t *hash_get_item(_string_t symbol) {
    item_t *item;
    hlist_for_each_entry(item, &symtable[hash_symbol(symbol)], hnode) {
        if (strcmp(item->common.symbol, symbol) == 0)
            return item;
    }
    return NULL;
}

static int hash_add_item(item_t *item, _string_t symbol) {
    /* ... check if symbol exists. */
    if (hash_get_item(symbol) != NULL)
        return -1;

    hlist_add_head(&item->hnode,
        &symtable[hash_symbol(symbol)]);

    return SUCCESS;
}

static token_t hash_get_token(_string_t symbol) {
    item_t *item;
    _token_list_t tp;

    if ((item = hash_get_item(symbol)) != NULL) {
        item_token_list_for_each(tp, item) {
            _extended_token_t etoken = container_of(tp, struct extended_token, node);

            if (etoken->flags & /* ... config or selected token. */
                (TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED))
                return etoken->token;
        }
    }

    return (token_t) {
        .ttype = TT_INVALID
    };
}

int push_menu(token_t token, _expr_t expr) {
    menu_t *menu;

    if ((menu = malloc(sizeof(menu_t))) != NULL) {
        menu->prompt = token.TK_STRING;
        menu->dependancy = expr;
        init_list_head(&menu->entries);
        init_list_head(&menu->childs);
        init_list_head(&menu->sibling);

        list_add_tail(&menu->sibling,
            &curr_menu->childs);

        curr_menu = menu;
        return SUCCESS;
    }

    return -1;
}

int pop_menu(void) {
    /* ... 'curr_menu' is tail of current menu. */
    curr_menu = container_of(curr_menu->sibling.next,
            menu_t, childs);
    return SUCCESS;
}

_token_list_t next_token(_token_list_t token1, unsigned long flags, ...) {
    va_list valist;
    _extended_token_t etoken;
    _token_list_t token = NULL;

    va_start(valist, flags);

    if ((etoken = malloc(sizeof(struct extended_token))) != NULL) {
        etoken->flags = flags;

        /* TODO 'flags' may be 'TK_LIST_EF_CONDITIONAL'.
         * Variable token list must be stored separately and evaluated
         * when processing an item. */

        if (flags == TK_LIST_EF_NULL ||
            flags & TK_LIST_EF_DEFAULT)
            etoken->token = va_arg(valist, token_t);

        token = token_list_add(&etoken->node, token1);
    }

    va_end(valist);

    return token;
}

void free_etoken(_extended_token_t e) {
    if (e->token.ttype == TT_SYMBOL ||
        e->token.ttype == TT_DESCRIPTION)
        free(e->token.TK_STRING);

    free(e);
}

static inline int init_entry(struct entry *entry,
    token_t prompt, token_t symbol,
    token_t help, _expr_t expr) {

    entry->prompt = prompt.TK_STRING;
    entry->symbol = symbol.TK_STRING;
    entry->dependancy = expr;
    entry->help = help.TK_STRING;

    return SUCCESS;
}

int add_new_config_entry(token_t token1, token_t token2,
    token_t token3, _token_list_t token4,
    _expr_t expr, token_t token5) {
    item_t *item;

    /* ... restrict 'select' keyword to only 'TT_BOOL'. */
    if (token4 != NULL && token3.ttype != TT_BOOL)
        return -1;

    if ((item = malloc(sizeof(item_t))) != NULL) {
        init_list_head(&item->list);
        init_hlist_node(&item->hnode);

        if (init_entry(&item->common, token1, token2, token5, expr) == -1)
            return -1;

        /* ... store 'token3' at the head as 'TK_LIST_EF_CONFIG'. */
        if ((item->tk_list = next_token(token4,
                        (TK_LIST_EF_CONFIG | TK_LIST_EF_DEFAULT), token3)) == NULL)
            return -1;

        item->refcount = 0;
        list_add_tail(&item->list, &curr_menu->entries);

        if (hash_add_item(item, token2.TK_STRING) == -1) {
            fprintf(stderr, "... %s symbol exists.\n", token2.TK_STRING);
            return -1;
        }

        return SUCCESS;
    }

    return -1;
}

int add_new_choice_entry(token_t token1, token_t token2,
    _token_list_t token3, _expr_t expr, token_t token4) {
    item_t *item;

    if ((item = malloc(sizeof(item_t))) != NULL) {
        init_list_head(&item->list);
        init_hlist_node(&item->hnode);

        if (init_entry(&item->common, token1, token2, token4, expr) == -1)
            return -1;

        item->tk_list = token3;
        list_add_tail(&item->list, &curr_menu->entries);

        if (hash_add_item(item, token2.TK_STRING) == -1) {
            fprintf(stderr, "... %s symbol exists.\n", token2.TK_STRING);
            return -1;
        }

        return SUCCESS;
    }

    return -1;
}

int add_new_config_file(token_t token1) {
    if ((config_files = next_token(config_files,
                    TK_LIST_EF_NULL, token1)) == NULL)
        return -1;

    return SUCCESS;
}

static _expr_t new_expr(enum expr_op op) {
    _expr_t expr;

    if ((expr = malloc(sizeof(struct expr))) != NULL)
        expr->op = op;

    return expr;
}

_expr_t expr_op_symbol_one(enum expr_op op, token_t token1) {
    _expr_t expr;

    if ((expr = new_expr(op)) != NULL)
        expr->NODE.token = token1;

    return expr;
}

_expr_t expr_op_symbol_two(enum expr_op op,
    token_t token1, token_t token2) {
    _expr_t expr;

    if ((expr = new_expr(op)) != NULL) {
        expr->LEFT.token = token1;
        expr->RIGHT.token = token2;
    }

    return expr;
}

_expr_t expr_op_expr_one(enum expr_op op, _expr_t expr1) {
    _expr_t expr;

    if ((expr = new_expr(op)) != NULL)
        expr->NODE.expr = expr1;

    return expr;
}

_expr_t expr_op_expr_two(enum expr_op op,
    _expr_t expr1, _expr_t expr2) {
    _expr_t expr;

    if ((expr = new_expr(op)) != NULL) {
        expr->LEFT.expr = expr1;
        expr->RIGHT.expr = expr2;
    }

    return expr;
}

static bool __eval_expr(token_t token1,
    token_t token2, enum expr_op op) {

    switch (token1.ttype) {
    case TT_BOOL:
        if (op == OP_EQUAL)
            return (token1.TK_BOOL == token2.TK_BOOL);

        if (op == OP_NEQUAL)
            return (token1.TK_BOOL != token2.TK_BOOL);

        break;

    case TT_INTEGER:
        if (op == OP_EQUAL)
            return (token1.TK_INTEGER == token2.TK_INTEGER);

        if (op == OP_NEQUAL)
            return (token1.TK_INTEGER != token2.TK_INTEGER);

        break;

    case TT_DESCRIPTION:
        if (op == OP_EQUAL)
            return (strcmp(token1.TK_STRING,
                        token2.TK_STRING) == 0);

        if (op == OP_NEQUAL)
            return (strcmp(token1.TK_STRING,
                        token2.TK_STRING) != 0);

        break;

    default:
        fprintf(stderr, "... unable to compute 'expr'.\n");
    }

    return false;
}

bool eval_expr(_expr_t expr) {
    item_t *item;
    _extended_token_t etoken;
    token_t token1, token2;

    /* ... no 'depends' keyword. */
    if (expr == NULL)
        return true;

    switch (expr->op) {
    case OP_NULL:
        if ((item = hash_get_item(expr->NODE.token.TK_STRING)) != NULL) {
            etoken = item_token_list_head_entry(item);

            /* ... make sure it is a config item. */
            if ((etoken->flags & TK_LIST_EF_CONFIG) &&
                (etoken->token.ttype == TT_BOOL)) {
                return etoken->token.TK_BOOL;
            }
        }

        fprintf(stderr, "... undefined or incompatible dependency: %s, assumed failed.\n",
            expr->NODE.token.TK_STRING);
        break;

    case OP_EQUAL:
    case OP_NEQUAL:
        /* ... use 'LEFT' and 'RIGHT' tokens. */
        token1 = expr->LEFT.token;
        token2 = expr->RIGHT.token;

        if (token1.ttype == TT_SYMBOL) {
            token1 = hash_get_token(token1.TK_STRING);

            if (token1.ttype == TT_INVALID) {
                fprintf(stderr, "... undefined dependency: %s, assumed failed.\n",
                    token1.TK_STRING);
                break;
            }
        }

        if (token2.ttype == TT_SYMBOL) {
            token2 = hash_get_token(token2.TK_STRING);

            if (token2.ttype == TT_INVALID) {
                fprintf(stderr, "... undefined dependency: %s, assumed failed.\n",
                    token2.TK_STRING);
                break;
            }
        }

        if (token1.ttype != token2.ttype) {
            fprintf(stderr, "... tokens are incompatible.\n");
            break;
        }

        return __eval_expr(token1, token2, expr->op);

    case OP_NOT:
        return !eval_expr(expr->NODE.expr);

    case OP_OR:
        return eval_expr(expr->LEFT.expr) ||
            eval_expr(expr->RIGHT.expr);

    case OP_AND:
        return eval_expr(expr->LEFT.expr) &&
            eval_expr(expr->RIGHT.expr);

    default:
        return false;
    }

    return false;
}

void __fprintf_menu(FILE *fp, menu_t *menu) {
    menu_t *m;
    item_t *item;

    if (eval_expr(menu->dependancy)) {
        /* ...  handle childs, first. */
        list_for_each_entry(m, &menu->childs, sibling) {
            __fprintf_menu(fp, m);
        }

        list_for_each_entry(item, &menu->entries, list) {
            if (eval_expr(item->common.dependancy)) {

                _token_list_t tp;
                item_token_list_for_each(tp, item) {
                    _extended_token_t etoken = container_of(tp, struct extended_token, node);

                    if (etoken->flags &
                        (TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED)) {
                        if (etoken->token.ttype == TT_BOOL) {
                            /* ... assume 'false' as undefined symbol. */
                            if (etoken->token.TK_BOOL == true)
                                fprintf(fp, "#define %s 1\n", item->common.symbol);

                        } else if (etoken->token.ttype == TT_INTEGER)
                            fprintf(fp, "#define %s %d\n", item->common.symbol,
                                etoken->token.TK_INTEGER);

                        else if (etoken->token.ttype  == TT_DESCRIPTION)
                            fprintf(fp, "#define %s %s\n", item->common.symbol,
                                etoken->token.TK_STRING);
                    }
                }
            }
        }
    }
}

int __populate_config_file(const char *filename, unsigned long flags) {
    item_t *item;
    _token_list_t tp;
    FILE *fp;

    int fd;

    if ((fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC |
                    /* ... file does not exist if dumping defaults. */
                    ((flags & TK_LIST_EF_DEFAULT) ? O_EXCL : 0),
                    S_IRUSR | S_IWUSR)) == -1)
        return -1;

    if ((fp = fdopen(fd, "w")) == NULL) {
        close(fd);
        return -1;
    }

    fprintf(fp, "# THIS IS AN AUTO-GENERATED FILE: DO NOT EDIT.\n");

    for (int i = 0; i < SYMTABLE; i++) {

        /* ... dump every items to be able to restore. */
        hlist_for_each_entry(item, &symtable[i], hnode) {
            fprintf(fp, "%s ", item->common.symbol);

            item_token_list_for_each(tp, item) {
                _extended_token_t etoken = container_of(tp, struct extended_token, node);

                if (etoken->flags & flags) {
                    if (etoken->token.ttype == TT_BOOL)
                        fprintf(fp, "%s\n",
                            etoken->token.TK_BOOL ? "true" : "false");

                    else if (etoken->token.ttype == TT_INTEGER)
                        fprintf(fp, "%d\n", etoken->token.TK_INTEGER);

                    else if (etoken->token.ttype  == TT_DESCRIPTION)
                        fprintf(fp, "%s\n", etoken->token.TK_STRING);

                    else
                        /* ... only when 'flag' is -1. */
                        fprintf(fp, "undefined.\n");

                    break;
                }
            }
        }
    }

    fclose(fp);
    return SUCCESS;
}

static void update_select_token_list(_token_list_t head, bool n) {
    item_t *item;
    _token_list_t tp;

    token_list_for_each(tp, head) { /* ... handle selects, if any. */
        _extended_token_t e, etoken = container_of(tp, struct extended_token, node);

        if ((item = hash_get_item(etoken->token.TK_STRING)) != NULL) {

            e = item_token_list_head_entry(item);

            if ((e->flags & TK_LIST_EF_CONFIG) &&
                (e->token.ttype == TT_BOOL)) {

                if (n == true) {
                    if (e->flags & TK_LIST_EF_DEFAULT ||
                        e->token.TK_BOOL == true)
                        item_inc(item);
                    else {
                        e->token.TK_BOOL = true;
                        update_select_token_list(item->tk_list->next, true);
                    }
                } else {
                    if (item->refcount > 0)
                        item_dec(item);

                    else if (e->token.TK_BOOL == true) {
                        e->token.TK_BOOL = false;
                        update_select_token_list(item->tk_list->next, false);
                    }
                }
            } else
                fprintf(stderr, "... incompatible select: %s, ignore.\n",
                    etoken->token.TK_STRING);

        } else
            fprintf(stderr, "... undefined select: %s, ignore.\n",
                etoken->token.TK_STRING);
    }

}

void __toggle_choice(_extended_token_t etoken, _string_t n) {
    if ((etoken->token.ttype == TT_INTEGER &&
            etoken->token.TK_INTEGER == atoi(n)) ||
        (etoken->token.ttype == TT_DESCRIPTION &&
            strcmp(etoken->token.TK_STRING, n) == 0)) {

        etoken->flags |= TK_LIST_EF_SELECTED;
    }
}

void toggle_config(item_t *item, ...) {
    va_list valist;
    _extended_token_t etoken;

    va_start(valist, item);
    etoken = item_token_list_head_entry(item);

    /* ... sure 'TK_LIST_EF_CONFIG' is set. */
    if (etoken->token.ttype == TT_BOOL) {
        etoken->token.TK_BOOL = !etoken->token.TK_BOOL;

        update_select_token_list(item->tk_list->next,
            etoken->token.TK_BOOL); /* and handle selects. */

    } else if (etoken->token.ttype == TT_INTEGER)
        etoken->token.TK_INTEGER = va_arg(valist, int);

    else { /* and TT_DESCRIPTION. */
        free(etoken->token.TK_STRING);
        etoken->token.TK_STRING = va_arg(valist, _string_t);
    }

    va_end(valist);
}

int read_config_file(const char *filename) {
    item_t *item;
    _token_list_t tp;

    FILE *fp;
    _string_t symbol = NULL, value;
    size_t n = 0;
    ssize_t llen;

    if ((fp = fopen(filename, "r")) == NULL)
        return -1;

    while ((llen = getline(&symbol, &n, fp)) != -1) {
        if (symbol[0] == '#')
            continue; /* ... ignore comments. */

        _string_t tmp = strstr(symbol, "\n");

        if (tmp != NULL) /* and get ride of delimiter. */
            tmp[0] = '\0';

        tmp = strstr(symbol, " ");
        tmp[0] = '\0';
        value = &tmp[1];

        if ((item = hash_get_item(symbol)) != NULL) {
            item_token_list_for_each(tp, item) {
                _extended_token_t etoken = container_of(tp, struct extended_token, node);

                if (etoken->flags & TK_LIST_EF_CONFIG) {
                    /* ... 'TK_LIST_EF_DEFAULT' is always set. */

                    if (etoken->token.ttype == TT_BOOL) {
                        int tmp = strncmp(value, "true", 4);

                        if (etoken->token.TK_BOOL == true) {

                            /* Here, 'refcount' represents number of times other
                             * configuration items selected this item, e.g. refcount == 2
                             * means this item has been selected 2 times so far.
                             *
                             * We only change 'TK_BOOL' to false if 'refcount' is zero.
                             * As 'TK_LIST_EF_DEFAULT' is set, i.e. it has not been processed before in
                             * this function, changing to false does not case any changes
                             * to other configuration items. */

                            if (tmp != 0) {
                                if (item->refcount > 0)
                                    item_dec(item);
                                else
                                    etoken->token.TK_BOOL = false;
                            }
                        } else {
                            if (tmp != 0) {
                                if (item->refcount > 0) {
                                    item_dec(item);
                                    etoken->token.TK_BOOL = true;
                                }
                            } else
                                etoken->token.TK_BOOL = true;
                        }

                        if (etoken->token.TK_BOOL == true)
                            update_select_token_list(tp->next, true);

                    } else if (etoken->token.ttype == TT_INTEGER)
                        etoken->token.TK_INTEGER = atoi(value);

                    else { /* and TT_DESCRIPTION. */
                        free(etoken->token.TK_STRING);
                        etoken->token.TK_STRING = strdup(value);
                    }

                    etoken->flags &= ~TK_LIST_EF_DEFAULT;
                    break; /* ... try next line. */
                } else
                    __toggle_choice(etoken, value);
            }
        } else
            fprintf(stderr, "... undefined symbol: %s, ingnore.\n", symbol);
    }

    fclose(fp);

    if (symbol != NULL)
        free(symbol);

    return SUCCESS;
}