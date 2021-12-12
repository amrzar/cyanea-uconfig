/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef char *_string_t;
typedef struct {
    int n, base;
} integer_t;
typedef struct {
    int ttype;
    union {
        bool boolean;
        integer_t number;
        _string_t string;
    } info;
} token_t;

#define TK_BOOL info.boolean
#define TK_INTEGER info.number.n
#define TK_STRING info.string

enum expr_op {
    OP_NULL = 1,
    OP_EQUAL,
    OP_NEQUAL,
    OP_OR,
    OP_AND,
    OP_NOT
};

struct expr {
    enum expr_op op;
    struct {
        union {
            token_t token;
            struct expr *expr;
        } up, down;
    } node;
};

#define LEFT node.up
#define NODE RIGHT
#define RIGHT node.down

/* ... entry's dependency tree. */
typedef struct expr *_expr_t;

struct token_list {
    struct token_list *next;
};

typedef struct token_list *_token_list_t;

#define token_list_add(new, head) ({        \
        (new)->next = (head); (new);        \
    })

#define token_list_for_each(pos, head)      \
    for (pos = (head); pos != NULL; pos = pos->next)

#endif /* __CONFIG_PARSER_H__ */