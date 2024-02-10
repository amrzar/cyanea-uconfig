/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <search.h>

#include "db.h"
#include "defaults.h"
#include "y.tab.h"

#define alloc(t) malloc(sizeof(t))

menu_t main_menu = {
    NULL,
    NULL,
    LIST_HEAD_INIT(main_menu.entries),
    LIST_HEAD_INIT(main_menu.childs),
    LIST_HEAD_INIT(main_menu.sibling)
};

menu_t *curr_menu = &main_menu;

LIST_HEAD files = LIST_HEAD_INIT(files);
LIST_HEAD symtable = LIST_HEAD_INIT(symtable);

static item_t *hash_get_item(string_t symbol)
{
    ENTRY entry = { symbol, NULL };
    ENTRY *found_entry = hsearch(entry, FIND);
    if (found_entry != NULL)
        return (item_t *) (found_entry->data);

    return NULL;
}

static int hash_add_item(item_t *item, string_t symbol)
{
    ENTRY entry = { symbol, item };

    if (hash_get_item(symbol) != NULL) {
        error_print("%s symbol exists.\n", symbol);
        return -1;
    }

    if (hsearch(entry, ENTER) == NULL) {
        error_print("''symtable'' is full.\n");
        return -1;
    }

    LIST_INSERT_TAIL(&item->sym_node, &symtable);
    return SUCCESS;
}

int push_menu(token_t token, expr_t expr)
{
    menu_t *menu = alloc(menu_t);

    if (menu == NULL) {
        error_print("''alloc'' fulled.\n");
        return -1;
    }

    menu->prompt = token.TK_STRING;
    menu->dependency = expr;
    INIT_LIST_HEAD(&menu->entries);
    INIT_LIST_HEAD(&menu->childs);
    INIT_LIST_HEAD(&menu->sibling);

    LIST_INSERT_TAIL(&menu->sibling, &curr_menu->childs);

    curr_menu = menu;
    return SUCCESS;
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
    struct extended_token *et;

    va_list va;
    va_start(va, flags);

    if ((et = alloc(struct extended_token)) != NULL) {
        et->flags = flags;
        et->condition = NULL;

        if (flags == TK_LIST_EF_NULL) {

            /* It is an entry in the list for 'select' keyword or a standard
             * (i.e. it is not default) option for 'choice' keyword. */

            et->token = va_arg(va, token_t);

        }

        if (flags & TK_LIST_EF_DEFAULT) {

            /* TODO. Check if there is any conditional token in the list. */

            /* It is the default option for 'choice' keyword. */

            et->token = va_arg(va, token_t);
        }

        if (flags & TK_LIST_EF_CONDITIONAL) {

            et->condition = va_arg(va, expr_t);
        }

        /* 'token1' is the tail of token list from last call. */

        token1 = token_list_add(&et->node, token1);

    } else {
        error_print("''alloc'' failed.\n");

        token1 = NULL;
    }

    va_end(va);

    return token1;
}

static inline int init_entry(struct item_shared *entry,
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

    item_t *item = alloc(item_t);

    if (item == NULL) {
        error_print("''alloc'' fulled.\n");
        return -1;
    }

    INIT_LIST_HEAD(&item->node);
    INIT_LIST_HEAD(&item->sym_node);
    init_entry(&item->common, token1, token2, token5, expr);

    /* Store 'token3' at the head as 'TK_LIST_EF_CONFIG'. */
    if ((item->tk_list =
                next_token(token4, (TK_LIST_EF_CONFIG | TK_LIST_EF_DEFAULT),
                    token3)) == NULL)
        return -1;

    item->refcount = 0;
    LIST_INSERT_TAIL(&item->node, &curr_menu->entries);

    if (hash_add_item(item, token2.TK_STRING) == -1) {
        error_print("%s symbol exists.\n", token2.TK_STRING);
        return -1;
    }

    return SUCCESS;
}

int add_new_choice_entry(token_t token1, token_t token2,
    struct token_list *token3, expr_t expr, token_t token4)
{
    item_t *item = alloc(item_t);

    if (item == NULL) {
        error_print("''alloc'' failed.\n");
        return -1;
    }

    INIT_LIST_HEAD(&item->node);
    INIT_LIST_HEAD(&item->sym_node);
    init_entry(&item->common, token1, token2, token4, expr);

    item->tk_list = token3;
    LIST_INSERT_TAIL(&item->node, &curr_menu->entries);

    if (hash_add_item(item, token2.TK_STRING) == -1) {
        error_print("%s symbol exists.\n", token2.TK_STRING);
        return -1;
    }

    return SUCCESS;
}

int add_new_config_file(token_t token1)
{
    struct include *file = alloc(struct include);

    if (file == NULL) {
        error_print("''alloc'' fulled.\n");
        return -1;
    }

    file->file = token1.TK_STRING;
    file->menu = curr_menu;
    LIST_INSERT_TAIL(&file->node, &files);

    return SUCCESS;
}

expr_t add_expr_op(enum expr_op op, ...)
{
    va_list va;
    expr_t expr = alloc(struct expr);

    if (expr == NULL) {
        error_print("''alloc'' fulled.\n");
        return NULL;
    }

    va_start(va, op);

    switch (op) {
    case OP_EQUAL:
    case OP_NEQUAL:
        expr->LEFT.token = va_arg(va, token_t);

    case OP_NULL:              /* 'RIGHT' is same as 'NODE' */
        expr->RIGHT.token = va_arg(va, token_t);
        break;

    case OP_AND:
    case OP_OR:
        expr->LEFT.expr = va_arg(va, expr_t);

    case OP_NOT:               /* 'RIGHT' is same as 'NODE' */
        expr->RIGHT.expr = va_arg(va, expr_t);
        break;

    default:
        free(expr);

        expr = NULL;
    }

    if (expr != NULL)
        expr->op = op;

    va_end(va);

    return expr;
}

static token_t hash_get_token(string_t symbol)
{
    struct extended_token *et;
    item_t *item = hash_get_item(symbol);

    if ((item != NULL) && eval_expr(item->common.dependency)) {
        item_token_list_for_each_entry(et, item) {
            if (et->flags & (TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED))
                return et->token;
        }
    }

    return ((token_t) {
        .ttype = TT_INVALID
    });
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
        } else
            break;

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

int fprintf_menu(FILE *fp, menu_t *menu)
{
    menu_t *m;
    item_t *item;

    if (!eval_expr(menu->dependency))
        return -1;

    /* Handle childs, first. */
    LIST_FOREACH(m, &menu->childs, sibling) {
        fprintf_menu(fp, m);
    }

    LIST_FOREACH(item, &menu->entries, node) {
        if (!eval_expr(item->common.dependency))
            continue;

        struct extended_token *et;
        item_token_list_for_each_entry(et, item) {

            if (et->flags & (TK_LIST_EF_CONFIG | TK_LIST_EF_SELECTED)) {
                if (et->token.ttype == TT_BOOL) {

                    /* Assume 'false' as undefined symbol. */
                    if (et->token.TK_BOOL == true)
                        fprintf(fp, "#define %s y\n", item->common.symbol);

                } else if (et->token.ttype == TT_INTEGER) {

                    /* There may be multiple options with 'TK_LIST_EF_SELECTED'. */

                    if (eval_expr(et->condition)) {
                        fprintf(fp, "#define %s %d\n", item->common.symbol,
                            et->token.TK_INTEGER);
                    }

                } else if (et->token.ttype == TT_DESCRIPTION) {
                    if (eval_expr(et->condition))
                        fprintf(fp, "#define %s \"%s\"\n", item->common.symbol,
                            et->token.TK_STRING);
                }
            }
        }

    }

    return SUCCESS;
}

int __populate_config_file(const char *filename, unsigned long flags)
{
    item_t *item;
    struct extended_token *et;
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
    LIST_FOREACH(item, &symtable, sym_node) {

        item_token_list_for_each_entry(et, item) {
            if (et->flags & flags) {
                switch (et->token.ttype) {
                case TT_BOOL:
                    fprintf(fp, "%s ", item->common.symbol);
                    fprintf(fp, "%s\n", et->token.TK_BOOL ? "true" : "false");
                    break;

                case TT_INTEGER:
                    fprintf(fp, "%s ", item->common.symbol);
                    fprintf(fp, "%d\n", et->token.TK_INTEGER);
                    break;

                case TT_DESCRIPTION:
                    fprintf(fp, "%s ", item->common.symbol);
                    fprintf(fp, "%s\n", et->token.TK_STRING);
                    break;

                default:
                    /* ... only when 'flag' is -1. */
                    fprintf(fp, "undefined.\n");

                }

                /* There may be multiple entries in the token list with the requested
                 * flags. We continue processing all tokes. For instance, multiple
                 * option in a 'choice' may have 'TK_LIST_EF_DEFAULT' set while
                 * being conditional, i.e. 'TK_LIST_EF_CONDITIONAL' set. */
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
        struct extended_token *e, *et = item_token_list_entry(tp);

        if ((item = hash_get_item(et->token.TK_STRING)) != NULL) {

            /* ... if entry is of 'TT_BOOL' type, toggle it as needed. */
            if (((e = item_get_config_et(item)) != NULL) &&
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
                debug_print("Incompatible select: %s.\n", et->token.TK_STRING);

        } else
            debug_print("Undefined select: %s.\n", et->token.TK_STRING);
    }

}

void __toggle_choice(struct extended_token *et, string_t n)
{
    if (((et->token.ttype == TT_INTEGER) &&
            /* ... use ''strtol'' as string ''n'' can start with '0x...' */
            (et->token.TK_INTEGER == strtol(n, NULL, 0))) ||
        ((et->token.ttype == TT_DESCRIPTION) &&
            (strcmp(et->token.TK_STRING, n) == 0))) {

        et->flags |= TK_LIST_EF_SELECTED;
    }
}

void toggle_config(item_t *item, ...)
{
    va_list va;
    struct extended_token *et;

    va_start(va, item);
    et = item_get_config_et(item);

    /* ... sure 'TK_LIST_EF_CONFIG' is set. */
    if (et->token.ttype == TT_BOOL) {
        et->token.TK_BOOL = !et->token.TK_BOOL;

        update_select_token_list(item->tk_list->next, et->token.TK_BOOL);

    } else if (et->token.ttype == TT_INTEGER)
        et->token.TK_INTEGER = va_arg(va, int);

    else {                      /* and TT_DESCRIPTION. */
        free(et->token.TK_STRING);
        et->token.TK_STRING = va_arg(va, string_t);
    }

    va_end(va);
}

int read_config_file(const char *filename)
{
    item_t *item;
    struct extended_token *et;

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

        item_token_list_for_each_entry(et, item) {

            if (et->flags & TK_LIST_EF_CONFIG) {
                /* ... 'TK_LIST_EF_DEFAULT' is always set. */

                if (et->token.ttype == TT_BOOL) {
                    int tmp = strncmp(value, "true", 4);

                    if (et->token.TK_BOOL == true) {

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
                                et->token.TK_BOOL = false;
                        }
                    } else {
                        if (tmp != 0) {
                            if (item->refcount > 0) {
                                item_dec(item);

                                /* We should leave this item as 'false', Here.
                                 * But as 'refcount' is already greater than zero,
                                 * this item has been selected before, so we set
                                 * it to 'true' instead. **/

                                et->token.TK_BOOL = true;
                            }
                        } else
                            et->token.TK_BOOL = true;
                    }

                    if (et->token.TK_BOOL == true)
                        update_select_token_list(et->node.next, true);

                } else if (et->token.ttype == TT_INTEGER)
                    et->token.TK_INTEGER = atoi(value);

                else {          /* and TT_DESCRIPTION. */
                    free(et->token.TK_STRING);
                    et->token.TK_STRING = strdup(value);
                }

                et->flags &= ~TK_LIST_EF_DEFAULT;
                break;          /* ... try next line. */
            } else
                __toggle_choice(et, value);
        }
    }

    fclose(fp);

    if (symbol != NULL)
        free(symbol);

    return SUCCESS;
}
