%{

#include "config.parser.h"
#include "config.db.h"

extern void yyerror(char *s);
extern int yylex (void);

extern int push_menu(token_t, _expr_t);
extern int pop_menu(void);

extern _expr_t add_expr_op(enum expr_op, ...);

#define yy_add_expr_op(op, ...) ({                      \
    _expr_t _tmp = add_expr_op((op), __VA_ARGS__);      \
    if ((_tmp) == NULL)                                 \
        YYERROR;                                        \
    (_tmp);                                             \
})

extern _token_list_t next_token(_token_list_t, unsigned long, ...);

#define yy_next_token(token1, flags, ...) ({            \
    _token_list_t _tmp = next_token((token1),           \
        (flags), __VA_ARGS__);                          \
    if ((_tmp) == NULL)                                 \
        YYERROR;                                        \
    (_tmp);                                             \
})

extern int add_new_config_entry(token_t, token_t, token_t, _token_list_t, _expr_t, token_t);
extern int add_new_choice_entry(token_t, token_t, _token_list_t, _expr_t, token_t);
extern int add_new_config_file(token_t);

#define NULLDESC (token_t) {                            \
    .ttype = TT_DESCRIPTION, .TK_STRING = NULL          \
}

%}

%union
{
    unsigned long flags;
    token_t token;
    _token_list_t tokenlist;
    _expr_t exprtree;
}

%start stmt_line

%token MENU
%token END
%token BOOL
%token INTEGER
%token STRING
%token HELP
%token DEPENDES
%token CONFIG
%token CHOICE
%token OPTION
%token SELECT
%token INCLUDE
%token DEFAULT

%token <token> TT_BOOL
%token <token> TT_INTEGER
%token <token> TT_SYMBOL
%token <token> TT_DESCRIPTION
%token <token> TT_INVALID

%type <token> help operand
%type <exprtree> expression dependency

%type <flags> choice_default
%type <tokenlist> choice_int_def choice_str_def
%type <tokenlist> choice_options

%type <tokenlist> config_selects
%type <token> config_type config_description

%token OPENPAREN
%token CLOSEPAREN

%left OR AND
%left EQUAL NEQUAL
%nonassoc NOT

%%

stmt_line: /* ... main entry to the grammer. */
    | menu_start stmt_line menu_end stmt_line
    | stmt_include stmt_line
    | stmt_config stmt_line
    | stmt_choice stmt_line
    ;

menu_start: MENU TT_DESCRIPTION dependency
{
    if (push_menu($2, $3) == -1)
        YYERROR;
};

menu_end: END
{
    if(pop_menu() == -1)
        YYERROR;
};

help:                       { $$ = NULLDESC;    } /* empty ... */
    | HELP TT_DESCRIPTION   { $$ = $2;          }
    ;

/* ... assume expressions constructed with ... */
operand: TT_BOOL | TT_INTEGER | TT_DESCRIPTION | TT_SYMBOL;

expression: TT_SYMBOL
        {   $$ = yy_add_expr_op(OP_NULL, $1);                 }
    | operand EQUAL operand
        {   $$ = yy_add_expr_op(OP_EQUAL, $1, $3);            }
    | operand NEQUAL operand
        {   $$ = yy_add_expr_op(OP_NEQUAL, $1, $3);           }
    | NOT expression
        {   $$ = yy_add_expr_op(OP_NOT, $2);                  }
    | expression AND expression
        {   $$ = yy_add_expr_op(OP_AND, $1, $3);              }
    | expression OR expression
        {   $$ = yy_add_expr_op(OP_OR, $1, $3);               }
    | OPENPAREN expression CLOSEPAREN { $$ = $2;              }
    ;

dependency:                 { $$ = NULL;        } /* empty . */
    | DEPENDES expression   { $$ = $2;          }
    ;

/* === THE 'CONFIG' SPECIFIC KEYWORD RULES === */

config_description:         { $$ = NULLDESC;    } /* empty . */
    | TT_DESCRIPTION        { $$ = $1;          }
    ;

config_selects:             { $$ = NULL;        } /* empty . */
    | SELECT TT_SYMBOL config_selects
        { $$ = yy_next_token($3, TK_LIST_EF_NULL, $2);        }
    ;

config_type: BOOL   { $$ = (token_t)  {
        .ttype = TT_BOOL,
        .TK_BOOL = true
        };
    }
    | BOOL TT_BOOL          { $$ = $2;          }
    | INTEGER TT_INTEGER    { $$ = $2;          }
    | STRING TT_DESCRIPTION { $$ = $2;          }
    ;

stmt_config: CONFIG config_description TT_SYMBOL
        config_type config_selects dependency help
{
    if (add_new_config_entry($2, $3,
            $4, $5, $6, $7) == -1)
        YYERROR;
};

/* === THE 'CHOICE' SPECIFIC KEYWORD RULES === */

choice_default:     { $$ = TK_LIST_EF_NULL;     } /* empty . */
    | DEFAULT       { $$ = TK_LIST_EF_DEFAULT;  }
    ;

choice_int_def:            { $$ = NULL;         } /* empty . */
    | OPTION TT_INTEGER choice_default choice_int_def
        { $$ = yy_next_token($4, $3, $2);       }
    ;

choice_str_def:            { $$ = NULL;         } /* empty . */
    | OPTION TT_DESCRIPTION choice_default choice_str_def
        { $$ = yy_next_token($4, $3, $2);       }
    ;

choice_options: /* ... options should have same type. */
      OPTION TT_INTEGER choice_default choice_int_def
        { $$ = yy_next_token($4, $3, $2);                     }
    | OPTION TT_DESCRIPTION choice_default choice_str_def
        { $$ = yy_next_token($4, $3, $2);                     }
;

stmt_choice: CHOICE TT_DESCRIPTION TT_SYMBOL
        choice_options dependency help
{
    if (add_new_choice_entry($2, $3,
            $4, $5, $6) == -1)
        YYERROR;
};

/* === THE 'INCLUDE' SPECIFIC KEYWORD RULES === */

stmt_include: INCLUDE TT_DESCRIPTION
{
    if (add_new_config_file($2) == -1)
        YYERROR;
};

%%
