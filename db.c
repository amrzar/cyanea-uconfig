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
    NULL,
    LIST_HEAD_INIT(main_menu.entries),
    LIST_HEAD_INIT(main_menu.childs),
    LIST_HEAD_INIT(main_menu.sibling)
};

menu_t *curr_menu = &main_menu;

struct list_head config_files = LIST_HEAD_INIT(config_files);
static struct hlist_head symtable[SYMTABLE];

void init_symbol_hash_table(void)
{
    for (int i = 0; i < SYMTABLE; i++)
        INIT_HLIST_HEAD(&symtable[i]);
}

static unsigned long hash_symbol(string_t symbol)
{
    int c;
    unsigned long hash = 5381UL;

    while ((c = *symbol++) != '\0')
        hash = ((hash << 5) + hash) + c;

    return (hash % SYMTABLE);
}

static item_t *hash_get_item(string_t symbol)
{
    item_t *item;
    hlist_for_each_entry(item, &symtable[hash_symbol(symbol)], node) {
        if (strcmp(item->common.symbol, symbol) == 0)
            return item;
    }

    return NULL;
}

static int hash_add_item(item_t * item, string_t symbol)
{
    if (hash_get_item(symbol) != NULL)
        return -1;

    hlist_add_head(&item->node, &symtable[hash_symbol(symbol)]);
    return SUCCESS;
}

int push_menu(token_t token, expr_t expr)
{
    menu_t *menu;

    if ((menu = malloc(sizeof(menu_t))) != NULL) {
        menu->prompt = token.TK_STRING;
        menu->dependency = expr;
        INIT_LIST_HEAD(&menu->entries);
        INIT_LIST_HEAD(&menu->childs);
        INIT_LIST_HEAD(&menu->sibling);

        list_add_tail(&menu->sibling, &curr_menu->childs);

        curr_menu = menu;
        return SUCCESS;
    }

    return -1;
}

int pop_menu(void)
{
    /* 'curr_menu' is tail of current menu. */
    curr_menu = container_of(curr_menu->sibling.next, menu_t, childs);

    return SUCCESS;
}

struct token_list *next_token(struct token_list *token1,
    unsigned long flags, ...)
{
    va_list valist;
    struct extended_token *etoken;

    va_start(valist, flags);

    if ((etoken = malloc(sizeof(struct extended_token))) != NULL) {
        etoken->flags = flags;

        /* TODO 'flags' may be 'TK_LIST_EF_CONDITIONAL'.
         * Condition for 'etoken' must be stored as a separate token list
         * retrieved from variable arguments. * */

        if ((flags == TK_LIST_EF_NULL) || (flags & TK_LIST_EF_DEFAULT))
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
    token_t prompt, token_t symbol, token_t help, expr_t expr)
{

    entry->prompt = prompt.TK_STRING;
    entry->symbol = symbol.TK_STRING;
    entry->dependency = expr;
    entry->help = help.TK_STRING;

    return SUCCESS;
}

int add_new_config_entry(token_t token1, token_t token2,
    token_t token3, struct token_list *token4, expr_t expr, token_t token5)
{
    /* Restrict 'select' keyword to only 'TT_BOOL'. */
    if ((token4 != NULL) && (token3.ttype != TT_BOOL))
        return -1;

    item_t *item = malloc(sizeof(item_t));

    if (item == NULL)
        return -1;

    INIT_LIST_HEAD(&item->list);
    INIT_HLIST_NODE(&item->node);

    if (init_entry(&item->common, token1, token2, token5, expr) == -1)
        return -1;

    /* Store 'token3' at the head as 'TK_LIST_EF_CONFIG'. */
    if ((item->tk_list =
            next_token(token4, (TK_LIST_EF_CONFIG | TK_LIST_EF_DEFAULT),
                token3)) == NULL)
        return -1;

    item->refcount = 0;
    list_add_tail(&item->list, &curr_menu->entries);

    if (hash_add_item(item, token2.TK_STRING) == -1) {
        error_print("%s symbol exists.\n", token2.TK_STRING);
        return -1;
    }

    return SUCCESS;
}

int add_new_choice_entry(token_t token1, token_t token2,
    struct token_list *token3, expr_t expr, token_t token4)
{
    item_t *item = malloc(sizeof(item_t));

    if (item == NULL)
        return -1;

    INIT_LIST_HEAD(&item->list);
    INIT_HLIST_NODE(&item->node);

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

int add_new_config_file(token_t token1)
{
    struct config_file *cfg_file = malloc(sizeof(struct config_file));

    if (cfg_file != NULL) {
        cfg_file->file = token1.TK_STRING;
        cfg_file->menu = curr_menu;

        list_add_tail(&cfg_file->node, &config_files);

        return SUCCESS;
    }

    return -1;
}

expr_t add_expr_op(enum expr_op op, ...)
{
    expr_t expr = malloc(sizeof(struct expr));

    if (expr == NULL)
        return NULL;

    va_list valist;
    va_start(valist, op);

    switch (op) {
    case OP_EQUAL:
    case OP_NEQUAL:
        expr->LEFT.token = va_arg(valist, token_t);

    case OP_NULL:              /* 'RIGHT' is same as 'NODE' */
        expr->RIGHT.token = va_arg(valist, token_t);
        break;

    case OP_AND:
    case OP_OR:
        expr->LEFT.expr = va_arg(valist, expr_t);

    case OP_NOT:               /* 'RIGHT' is same as 'NODE' */
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

static token_t hash_get_token(string_t symbol)
{
    struct extended_token *etoken;
    item_t *item = hash_get_item(symbol);

    if ((item != NULL) && eval_expr(item->common.dependency)) {
        item_token_list_for_each_entry(etoken, item) {
            if (etoken->flags & (TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED))
                return etoken->token;
        }
    }

    return ((token_t) {
        .ttype = TT_INVALID}
    );
}

static bool __eval_expr(token_t token1, token_t token2, enum expr_op op)
{
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
            return (strcmp(token1.TK_STRING, token2.TK_STRING) == 0);

        if (op == OP_NEQUAL)
            return (strcmp(token1.TK_STRING, token2.TK_STRING) != 0);

        break;

    default:
        debug_print("Unable to compute 'expr'.\n");
    }

    return false;
}

bool eval_expr(expr_t expr)
{
    token_t token, token1, token2;

    /* No 'depends' keyword, always success. */
    if (expr == NULL)
        return true;

    switch (expr->op) {
    case OP_NULL:
        token = expr->NODE.token;

        if (token.ttype == TT_SYMBOL) {
            token = hash_get_token(token.TK_STRING);

            if ((token.ttype == TT_INVALID) || (token.ttype != TT_BOOL)) {
                debug_print("Broken dependency: %s.\n", token.TK_STRING);
                break;
            }
        }

        return token.TK_BOOL;

    case OP_EQUAL:
    case OP_NEQUAL:
        /* Use 'LEFT' and 'RIGHT' tokens. */
        token1 = expr->LEFT.token;
        token2 = expr->RIGHT.token;

        if (token1.ttype == TT_SYMBOL) {
            token1 = hash_get_token(token1.TK_STRING);

            if (token1.ttype == TT_INVALID) {
                debug_print("Broken dependency: %s.\n", token1.TK_STRING);
                break;
            }
        }

        if (token2.ttype == TT_SYMBOL) {
            token2 = hash_get_token(token2.TK_STRING);

            if (token2.ttype == TT_INVALID) {
                debug_print("Broken dependency: %s.\n", token2.TK_STRING);
                break;
            }
        }

        if (token1.ttype != token2.ttype) {
            debug_print("Tokens are incompatible.\n");
            break;
        }

        return __eval_expr(token1, token2, expr->op);

    case OP_NOT:
        return !eval_expr(expr->NODE.expr);

    case OP_OR:
        return eval_expr(expr->LEFT.expr) || eval_expr(expr->RIGHT.expr);

    case OP_AND:
        return eval_expr(expr->LEFT.expr) && eval_expr(expr->RIGHT.expr);

    default:
        return false;
    }

    return false;
}

int fprintf_menu(FILE * fp, menu_t * menu)
{
    menu_t *m;
    item_t *item;

    if (!eval_expr(menu->dependency))
        return -1;

    /* Handle childs, first. */
    list_for_each_entry(m, &menu->childs, sibling) {
        fprintf_menu(fp, m);
    }

    list_for_each_entry(item, &menu->entries, list) {
        if (!eval_expr(item->common.dependency))
            continue;

        struct extended_token *etoken;
        item_token_list_for_each_entry(etoken, item) {

            if (etoken->flags & (TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED)) {
                if (etoken->token.ttype == TT_BOOL) {

                    /* Assume 'false' as undefined symbol. */
                    if (etoken->token.TK_BOOL == true)
                        fprintf(fp, "#define %s y\n", item->common.symbol);

                } else if (etoken->token.ttype == TT_INTEGER) {
                    fprintf(fp, "#define %s %d\n", item->common.symbol,
                        etoken->token.TK_INTEGER);

                } else if (etoken->token.ttype == TT_DESCRIPTION) {
                    fprintf(fp, "#define %s %s\n", item->common.symbol,
                        etoken->token.TK_STRING);
                }
            }
        }

    }

    return SUCCESS;
}

int __populate_config_file(const char *filename, unsigned long flags)
{
    item_t *item;
    struct extended_token *etoken;
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

    /* Dump every items to 'fp' based on 'flags'. */
    for (int i = 0; i < SYMTABLE; i++) {
        hlist_for_each_entry(item, &symtable[i], node) {
            fprintf(fp, "%s ", item->common.symbol);

            item_token_list_for_each_entry(etoken, item) {

                if (etoken->flags & flags) {
                    switch (etoken->token.ttype) {
                    case TT_BOOL:
                        fprintf(fp, "%s\n",
                            etoken->token.TK_BOOL ? "true" : "false");
                        break;

                    case TT_INTEGER:
                        fprintf(fp, "%d\n", etoken->token.TK_INTEGER);
                        break;

                    case TT_DESCRIPTION:
                        fprintf(fp, "%s\n", etoken->token.TK_STRING);
                        break;

                    default:
                        /* ... only when 'flag' is -1. */
                        fprintf(fp, "undefined.\n");

                    }

                    break;
                }
            }
        }
    }

    fclose(fp);
    return SUCCESS;
}

static void update_select_token_list(struct token_list *head, bool n)
{
    item_t *item;
    struct token_list *tp;

    /* Handle entries in 'selects' list, if any ... */
    token_list_for_each(tp, head) {
        struct extended_token *e, *etoken = item_token_list_entry(tp);

        if ((item = hash_get_item(etoken->token.TK_STRING)) != NULL) {

            /* ... if entry is of 'TT_BOOL' type, toggle it as needed. */
            if (((e = item_get_config_etoken(item)) != NULL) &&
                (e->token.ttype == TT_BOOL)) {

                if (n == true) {

                    /* If 'TK_LIST_EF_DEFAULT' is set, then we have not
                     * processed this item in 'read_config_file' yet. We do
                     * not update the state to 'true' even if it is 'false'.
                     * 'read_config_file' should handle it. **/

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
                debug_print("Incompatible select: %s.\n",
                    etoken->token.TK_STRING);

        } else
            debug_print("Undefined select: %s.\n", etoken->token.TK_STRING);
    }

}

void __toggle_choice(struct extended_token *etoken, string_t n)
{
    if (((etoken->token.ttype == TT_INTEGER) &&
            (etoken->token.TK_INTEGER == atoi(n))) ||
        ((etoken->token.ttype == TT_DESCRIPTION) &&
            (strcmp(etoken->token.TK_STRING, n) == 0))) {

        etoken->flags |= TK_LIST_EF_SELECTED;
    }
}

void toggle_config(item_t * item, ...)
{
    va_list valist;
    struct extended_token *etoken;

    va_start(valist, item);
    etoken = item_get_config_etoken(item);

    /* ... sure 'TK_LIST_EF_CONFIG' is set. */
    if (etoken->token.ttype == TT_BOOL) {
        etoken->token.TK_BOOL = !etoken->token.TK_BOOL;

        update_select_token_list(item->tk_list->next, etoken->token.TK_BOOL);

    } else if (etoken->token.ttype == TT_INTEGER)
        etoken->token.TK_INTEGER = va_arg(valist, int);

    else {                      /* and TT_DESCRIPTION. */
        free(etoken->token.TK_STRING);
        etoken->token.TK_STRING = va_arg(valist, string_t);
    }

    va_end(valist);
}

int read_config_file(const char *filename)
{
    item_t *item;
    struct extended_token *etoken;

    FILE *fp;
    string_t symbol = NULL, value;
    size_t n = 0;

    if ((fp = fopen(filename, "r")) == NULL)
        return -1;

    while (getline(&symbol, &n, fp) != -1) {
        if (symbol[0] == '#')
            continue;

        string_t tmp = strstr(symbol, "\n");

        if (tmp != NULL)        /* and get ride of delimiter. */
            tmp[0] = '\0';

        tmp = strstr(symbol, " ");
        tmp[0] = '\0';
        value = &tmp[1];

        if ((item = hash_get_item(symbol)) == NULL) {
            debug_print("Undefined symbol: %s.\n", symbol);
            continue;
        }

        item_token_list_for_each_entry(etoken, item) {

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
                         * As 'TK_LIST_EF_DEFAULT' is set, i.e. it has not been
                         * processed before in this function, changing to 'false'
                         * does not case any changes to other configuration items. **/

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

                                /* We should leave this item as 'false', Here.
                                 * But as 'refcount' is already greater than zero,
                                 * this item has been selected before, so we set
                                 * it to 'true' instead. **/

                                etoken->token.TK_BOOL = true;
                            }
                        } else
                            etoken->token.TK_BOOL = true;
                    }

                    if (etoken->token.TK_BOOL == true)
                        update_select_token_list(etoken->node.next, true);

                } else if (etoken->token.ttype == TT_INTEGER)
                    etoken->token.TK_INTEGER = atoi(value);

                else {          /* and TT_DESCRIPTION. */
                    free(etoken->token.TK_STRING);
                    etoken->token.TK_STRING = strdup(value);
                }

                etoken->flags &= ~TK_LIST_EF_DEFAULT;
                break;          /* ... try next line. */
            } else
                __toggle_choice(etoken, value);
        }
    }

    fclose(fp);

    if (symbol != NULL)
        free(symbol);

    return SUCCESS;
}
