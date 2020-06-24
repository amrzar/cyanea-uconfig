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

#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

typedef char *_string_t;
typedef struct {
    int ttype;
    union {
        bool b;
        int n;
        _string_t s;
    } piggy;
} token_t;

#define TK_BOOL piggy.b
#define TK_INTEGER piggy.n
#define TK_STRING piggy.s

#define TOKEN_INVALID (token_t) \
    { .ttype = TT_INVALID }

#define NULLDESC (token_t)      \
    { .ttype = TT_DESCRIPTION, .TK_STRING = NULL }

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
        union __e {
            token_t token;
            struct expr *expr;
        } up;
        union __e down;
    } node;
};

#define LEFT node.up
#define NODE LEFT
#define RIGHT node.down

/* ... entry's dependancy tree. */
typedef struct expr *_expr_t;

struct token_list {
    struct token_list *next;
};

typedef struct token_list *_token_list_t;

#define token_list_add(new, head) ({        \
        (new)->next = (head); (new);            \
    })

#define token_list_for_each(pos, head)      \
    for (pos = (head); pos != NULL; pos = pos->next)

#endif /* __CONFIG_PARSER_H__ */