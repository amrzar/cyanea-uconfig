%{

#include "config.parser.h"
#include "config.db.h"

extern void yyerror(char *s);
extern int yylex (void);

#ifdef DEBUG
int yydebug = 1;
#endif

/* ... 'ptr' is not NULL. */
#define YYTEST(_ptr) ({ 		\
	void *_tmp = (_ptr);		\
	if ((_tmp) == NULL)			\
		YYERROR; 				\
	(_tmp); 					\
})

#define NULLDESC (token_t) { 	\
	.ttype = TT_DESCRIPTION, 	\
	.TK_STRING = NULL 			\
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

%type <token> stmt_help exprin
%type <exprtree> stmt_expression stmt_dependency

%type <flags> choice_default
%type <tokenlist> choice_int_def choice_str_def
%type <tokenlist> stmt_choice_options

%type <tokenlist> stmt_config_selects 
%type <token> stmt_config_type stmt_config_description 

%token OPENPAREN
%token CLOSEPAREN

%left OR AND
%left EQUAL NEQUAL
%nonassoc NOT

%%

stmt_line: /* ... main entry to the grammer. */
	| stmt_menu_start stmt_line stmt_menu_end stmt_line
	| stmt_include stmt_line
	| stmt_config stmt_line
	| stmt_choice stmt_line
;

stmt_menu_start: MENU TT_DESCRIPTION stmt_dependency
{
	if (push_menu($2, $3) == -1)
		YYERROR;
};

stmt_menu_end: END
{
	if(pop_menu() == -1)
		YYERROR;
};

stmt_help: /* ... 'help' is optional. */
		{ $$ = NULLDESC; }
	| HELP TT_DESCRIPTION
		{ $$ = $2; }
;

/* ... assume expressions constructed with ... */
exprin: TT_BOOL | TT_INTEGER | TT_DESCRIPTION | TT_SYMBOL;

stmt_expression: TT_SYMBOL
		{ $$ = YYTEST(expr_op_symbol_one(OP_NULL, $1));	}
	| exprin EQUAL exprin /* ... 'exprin' type-checking in C. */
		{ $$ = YYTEST(expr_op_symbol_two(OP_EQUAL, $1, $3)); }
	| exprin NEQUAL exprin /* ... 'exprin' type-checking in C. */
		{ $$ = YYTEST(expr_op_symbol_two(OP_NEQUAL, $1, $3)); }
	| NOT stmt_expression
		{ $$ = YYTEST(expr_op_expr_one(OP_NOT, $2)); }
	| stmt_expression AND stmt_expression
		{ $$ = YYTEST(expr_op_expr_two(OP_AND, $1, $3)); }
	| stmt_expression OR stmt_expression
		{ $$ = YYTEST(expr_op_expr_two(OP_OR, $1, $3));	}
	| OPENPAREN stmt_expression CLOSEPAREN
		{ $$ = $2; }
;

stmt_dependency: /* ... 'depends' is optional. */
		{ $$ = NULL; }
	| DEPENDES stmt_expression
		{ $$ = $2; }
;

/* ... THE 'CONFIG' SPECIFIC KEYWORD RULES. */

stmt_config_description: /* ... is optional. */
		{ $$ = NULLDESC; }
	| TT_DESCRIPTION
		{ $$ = $1; }
;

stmt_config_selects: /* ... is optional. */
		{ $$ = NULL; }
	| SELECT TT_SYMBOL stmt_config_selects
		{ $$ = YYTEST(next_token($3, TK_LIST_EF_NULL, $2));	}
;

stmt_config_type: BOOL TT_BOOL
		{ $$ = $2; }
	| INTEGER TT_INTEGER
		{ $$ = $2; }
	| STRING TT_DESCRIPTION
		{ $$ = $2; }
;

stmt_config: CONFIG stmt_config_description TT_SYMBOL
		stmt_config_type stmt_config_selects stmt_dependency stmt_help
{
	if (add_new_config_entry($2, $3,
			$4, $5, $6, $7) == -1)
		YYERROR;
};

/* ... THE 'CHOICE' SPECIFIC KEYWORD RULES. */

choice_default: /* ... is optional. */
		{ $$ = TK_LIST_EF_NULL;	}
	| DEFAULT
		{ $$ = TK_LIST_EF_DEFAULT; }

choice_int_def: /* ... is optional. */
		{ $$ = NULL; }
	| OPTION TT_INTEGER choice_default choice_int_def 
		{ $$ = YYTEST(next_token($4, $3, $2)); }
;

choice_str_def: /* ... is optional. */
		{ $$ = NULL; }
	| OPTION TT_DESCRIPTION choice_default choice_str_def
		{ $$ = YYTEST(next_token($4, $3, $2)); }
;

stmt_choice_options: /* ... options should have same type. */
	  OPTION TT_INTEGER choice_default choice_int_def
		{ $$ = YYTEST(next_token($4, $3, $2)); }
	| OPTION TT_DESCRIPTION choice_default choice_str_def
		{ $$ = YYTEST(next_token($4, $3, $2)); }
;

stmt_choice: CHOICE TT_DESCRIPTION TT_SYMBOL
		stmt_choice_options stmt_dependency stmt_help
{
	if (add_new_choice_entry($2, $3,
			$4, $5, $6) == -1)
		YYERROR;
};

/* ... THE 'INCLUDE' SPECIFIC KEYWORD RULES. */

stmt_include: INCLUDE TT_DESCRIPTION
{
	if (add_new_config_file($2) == -1)
		YYERROR;
};

%%
