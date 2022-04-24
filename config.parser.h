/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef char *string_t;

typedef struct {
    int n, base;
} integer_t;

typedef struct {
    int ttype;
    union {
        bool boolean;
        integer_t number;
        string_t string;
    } info;
#define TK_BOOL info.boolean
#define TK_INTEGER info.number.n
#define TK_STRING info.string
} token_t;

enum expr_op {
    OP_NULL = 1,
    OP_EQUAL,
    OP_NEQUAL,
    OP_OR,
    OP_AND,
    OP_NOT
};

typedef struct expr *expr_t;
struct expr {
    enum expr_op op;
    struct {
        union {
            token_t token;
            expr_t expr;
        } up, down;
    } node;
#define LEFT node.up
#define NODE RIGHT
#define RIGHT node.down
};

/*
 * Single pointer linked list 'struct token_list'.
 * It is used with 'token_t' to store the list of parsed tokens from config files.
 * See 'struct extended_token' in db.h file.
 *
 **/

typedef struct token_list *_token_list_t;
struct token_list {
    _token_list_t next;
};

#define token_list_add(new, head) ({        \
        (new)->next = (head); (new);        \
    })

#define token_list_for_each(pos, head)      \
    for (pos = (head); pos != NULL; pos = pos->next)

#endif /* __CONFIG_PARSER_H__ */