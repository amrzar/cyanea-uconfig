/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>

#include "db.h"
#include "y.tab.h"

#define SYMTABLE 256

menu_t main_menu = {
    NULL,
    NULL, /* ... always true. */
    LIST_HEAD_INIT(main_menu.entries),
    LIST_HEAD_INIT(main_menu.childs),
    LIST_HEAD_INIT(main_menu.sibling)
};

menu_t *curr_menu = &main_menu;

struct list_head config_files = LIST_HEAD_INIT(config_files);
static struct hlist_head symtable[SYMTABLE];

void init_symbol_hash_table(void) {
    for (int i = 0; i < SYMTABLE; i++)
        INIT_HLIST_HEAD(&symtable[i]);
}

static unsigned long hash_symbol(string_t symbol) {
    int c;
    unsigned long hash = 5381;

    while ((c = *symbol++) != '\0')
        hash = ((hash << 5) + hash) + c;

    return hash % SYMTABLE;
}

static item_t *hash_get_item(string_t symbol) {
    item_t *item;
    hlist_for_each_entry(item, &symtable[hash_symbol(symbol)], hnode) {
        if (strcmp(item->common.symbol, symbol) == 0)
            return item;
    }
    return NULL;
}

static int hash_add_item(item_t *item, string_t symbol) {
    /* ... check if symbol exists. */
    if (hash_get_item(symbol) != NULL)
        return -1;

    hlist_add_head(&item->hnode,
        &symtable[hash_symbol(symbol)]);

    return SUCCESS;
}

static token_t hash_get_token(string_t symbol) {
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

int push_menu(token_t token, expr_t expr) {
    menu_t *menu;

    if ((menu = malloc(sizeof(menu_t))) != NULL) {
        menu->prompt = token.TK_STRING;
        menu->dependency = expr;
        INIT_LIST_HEAD(&menu->entries);
        INIT_LIST_HEAD(&menu->childs);
        INIT_LIST_HEAD(&menu->sibling);

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

    va_start(valist, flags);

    if ((etoken = malloc(sizeof(struct extended_token))) != NULL) {
        etoken->flags = flags;

        /* TODO 'flags' may be 'TK_LIST_EF_CONDITIONAL'.
         * Condition for 'etoken' must be stored as a separate token list
         * retrieved from variable arguments. */

        if (flags == TK_LIST_EF_NULL ||
            flags & TK_LIST_EF_DEFAULT)
            etoken->token = va_arg(valist, token_t);

        /* 'token1' is the tail of token list from last call. */

        token1 = token_list_add(&etoken->node, token1);
    } else {
        token1 = NULL;
    }

    va_end(valist);

    return token1;
}

static inline int init_entry(struct entry *entry,
    token_t prompt, token_t symbol,
    token_t help, expr_t expr) {

    entry->prompt = prompt.TK_STRING;
    entry->symbol = symbol.TK_STRING;
    entry->dependency = expr;
    entry->help = help.TK_STRING;

    return SUCCESS;
}

int add_new_config_entry(token_t token1, token_t token2,
    token_t token3, _token_list_t token4,
    expr_t expr, token_t token5) {
    item_t *item;

    /* ... restrict 'select' keyword to only 'TT_BOOL'. */
    if (token4 != NULL && token3.ttype != TT_BOOL)
        return -1;

    if ((item = malloc(sizeof(item_t))) != NULL) {
        INIT_LIST_HEAD(&item->list);
        INIT_HLIST_NODE(&item->hnode);

        if (init_entry(&item->common, token1, token2, token5, expr) == -1)
            return -1;

        /* ... store 'token3' at the head as 'TK_LIST_EF_CONFIG'. */
        if ((item->tk_list = next_token(token4,
                        (TK_LIST_EF_CONFIG | TK_LIST_EF_DEFAULT), token3)) == NULL)
            return -1;

        item->refcount = 0;
        list_add_tail(&item->list, &curr_menu->entries);

        if (hash_add_item(item, token2.TK_STRING) == -1) {
            error_print("%s symbol exists.\n", token2.TK_STRING);
            return -1;
        }

        return SUCCESS;
    }

    return -1;
}

int add_new_choice_entry(token_t token1, token_t token2,
    _token_list_t token3, expr_t expr, token_t token4) {
    item_t *item;

    if ((item = malloc(sizeof(item_t))) != NULL) {
        INIT_LIST_HEAD(&item->list);
        INIT_HLIST_NODE(&item->hnode);

        if (init_entry(&item->common, token1, token2, token4, expr) == -1)
            return -1;

        item->tk_list = token3;
        list_add_tail(&item->list, &curr_menu->entries);

        if (hash_add_item(item, token2.TK_STRING) == -1) {
            error_print("%s symbol exists.\n", token2.TK_STRING);
            return -1;
        }

        return SUCCESS;
    }

    return -1;
}

int add_new_config_file(token_t token1) {
    _config_file_t cfg_file;

    if ((cfg_file = malloc(sizeof(struct config_file))) != NULL) {
        cfg_file->file = token1.TK_STRING;
        cfg_file->menu = curr_menu;

        list_add_tail(&cfg_file->node, &config_files);

        return SUCCESS;
    }

    return -1;
}

expr_t add_expr_op(enum expr_op op, ...) {
    va_list valist;
    expr_t expr;

    if ((expr = malloc(sizeof(struct expr))) == NULL) {
        return NULL;
    }

    va_start(valist, op);

    switch (op) {
    case OP_EQUAL:
    case OP_NEQUAL:
        expr->LEFT.token = va_arg(valist, token_t);

    case OP_NULL: /* 'RIGHT' is same as 'NODE' */
        expr->RIGHT.token = va_arg(valist, token_t);
        break;

    case OP_AND:
    case OP_OR:
        expr->LEFT.expr = va_arg(valist, expr_t);

    case OP_NOT: /* 'RIGHT' is same as 'NODE' */
        expr->RIGHT.expr = va_arg(valist, expr_t);
        break;

    default:
        free(expr);

        expr = NULL;
    }

    if (expr != NULL)
        expr->op = op;

    va_end(valist);

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
        debug_print("unable to compute 'expr'.\n");
    }

    return false;
}

bool eval_expr(expr_t expr) {
    item_t *item;
    _extended_token_t etoken;
    token_t token1, token2;

    /* No 'depends' keyword, always success. */
    if (expr == NULL)
        return true;

    switch (expr->op) {
    case OP_NULL:
        if ((item = hash_get_item(expr->NODE.token.TK_STRING)) != NULL) {
            if (((etoken = item_get_config_etoken(item)) != NULL) &&
                (etoken->token.ttype == TT_BOOL)) {
                return etoken->token.TK_BOOL;
            }
        }

        debug_print("undefined or incompatible dependency: %s, assumed failed.\n",
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
                debug_print("undefined dependency: %s, assumed failed.\n", token1.TK_STRING);
                break;
            }
        }

        if (token2.ttype == TT_SYMBOL) {
            token2 = hash_get_token(token2.TK_STRING);

            if (token2.ttype == TT_INVALID) {
                debug_print("undefined dependency: %s, assumed failed.\n", token2.TK_STRING);
                break;
            }
        }

        if (token1.ttype != token2.ttype) {
            debug_print("tokens are incompatible.\n");
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

void fprintf_menu(FILE *fp, menu_t *menu) {
    menu_t *m;
    item_t *item;

    if (eval_expr(menu->dependency)) {
        /* ... handle childs, first. */
        list_for_each_entry(m, &menu->childs, sibling) {
            fprintf_menu(fp, m);
        }

        list_for_each_entry(item, &menu->entries, list) {
            if (eval_expr(item->common.dependency)) {

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

            if (((e = item_get_config_etoken(item)) != NULL) &&
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
                debug_print("incompatible select: %s, ignore.\n",
                    etoken->token.TK_STRING);

        } else
            debug_print("undefined select: %s, ignore.\n",
                etoken->token.TK_STRING);
    }

}

void __toggle_choice(_extended_token_t etoken, string_t n) {
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
    etoken = item_get_config_etoken(item);

    /* ... sure 'TK_LIST_EF_CONFIG' is set. */
    if (etoken->token.ttype == TT_BOOL) {
        etoken->token.TK_BOOL = !etoken->token.TK_BOOL;

        update_select_token_list(item->tk_list->next,
            etoken->token.TK_BOOL); /* and handle selects. */

    } else if (etoken->token.ttype == TT_INTEGER)
        etoken->token.TK_INTEGER = va_arg(valist, int);

    else { /* and TT_DESCRIPTION. */
        free(etoken->token.TK_STRING);
        etoken->token.TK_STRING = va_arg(valist, string_t);
    }

    va_end(valist);
}

int read_config_file(const char *filename) {
    item_t *item;
    _token_list_t tp;

    FILE *fp;
    string_t symbol = NULL, value;
    size_t n = 0;

    if ((fp = fopen(filename, "r")) == NULL)
        return -1;

    while (getline(&symbol, &n, fp) != -1) {
        if (symbol[0] == '#')
            continue; /* ... ignore comments. */

        string_t tmp = strstr(symbol, "\n");

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
                             * configuration items selected this item, e.g. 'refcount == 2'
                             * means this item has been selected 2 times so far.
                             *
                             * We only change 'TK_BOOL' to false if 'refcount' is zero.
                             * As 'TK_LIST_EF_DEFAULT' is set, i.e. it has not been processed before in
                             * this function, changing to 'false' does not case any changes
                             * to other configuration items.
                             *
                             **/

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

                                    /* We should leave this item as 'false', Here. But as 'refcount' is
                                     * already greater than zero, this item has been selected before, so
                                     * we set it to 'true' instead.
                                     *
                                     **/

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
            debug_print("undefined symbol: %s, ingnore.\n", symbol);
    }

    fclose(fp);

    if (symbol != NULL)
        free(symbol);

    return SUCCESS;
}