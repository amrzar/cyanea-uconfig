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

%}

%union 
{
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
%token DEFAULT

%token <token> TT_BOOL
%token <token> TT_INTEGER
%token <token> TT_SYMBOL
%token <token> TT_DESCRIPTION
%token <token> TT_INVALID

%type <token> stmt_help exprin
%type <exprtree> stmt_expression stmt_dependency

%type <tokenlist> choice_int_def choice_int_ndef
%type <tokenlist> choice_str_def choice_str_ndef
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
		  stmt_menu_start stmt_menu_end
		| stmt_menu_start stmt_line stmt_menu_end
		| stmt_config stmt_line
		| stmt_choice stmt_line
		| stmt_config
		| stmt_choice
;

stmt_menu_start: MENU TT_DESCRIPTION stmt_dependency
{
	if ($2.ttype != TT_DESCRIPTION ||
		push_menu($2, $3) == -1)
		YYERROR;
};

stmt_menu_end: END
{
	if(pop_menu() == -1)
		YYERROR;
};

stmt_help: 
		  /* optional: empty */ 				{ $$ = NULLDESC; 	}
		| HELP TT_DESCRIPTION
{	
	/* ... generally, these checks are redundant. */
	if ($2.ttype != TT_DESCRIPTION)
		YYERROR;
	$$ = $2;
};

/* ... assume expressions constructed with ... */
exprin: TT_BOOL | TT_INTEGER | TT_DESCRIPTION | TT_SYMBOL;

stmt_expression: 
		  TT_SYMBOL
{
	_expr_t expr;
	if ($1.ttype != TT_SYMBOL ||
		((expr = expr_op_symbol_one(OP_NULL, $1)) == NULL))
		YYERROR;
	$$ = expr;
}
		/* ... pass 'exprin' and user handles type-checking. */
		| exprin EQUAL exprin
			{ $$ = YYTEST(expr_op_symbol_two(OP_EQUAL, $1, $3));	}
		| exprin NEQUAL exprin
			{ $$ = YYTEST(expr_op_symbol_two(OP_NEQUAL, $1, $3));	}

		| NOT stmt_expression
			{ $$ = YYTEST(expr_op_expr_one(OP_NOT, $2)); 	 		}
		| stmt_expression AND stmt_expression
			{ $$ = YYTEST(expr_op_expr_two(OP_AND, $1, $3)); 		}
		| stmt_expression OR stmt_expression
			{ $$ = YYTEST(expr_op_expr_two(OP_OR, $1, $3));  		}
		| OPENPAREN stmt_expression CLOSEPAREN 		{ $$ = $2; 		}
;

stmt_dependency:
		  /* optional: empty */						{ $$ = NULL; 	}
		| DEPENDES stmt_expression 					{ $$ = $2; 		}
;

/* ... THE 'CONFIG' SPECIFIC KEYWORD RULES. */

stmt_config_description:
		  /* empty symbol. */ 			 		{ $$ = NULLDESC; 	}
		| TT_DESCRIPTION
{
	if ($1.ttype != TT_DESCRIPTION)
		YYERROR;
	$$ = $1;
};

stmt_config_selects:
		  /* optional: empty. */ 					{ $$ = NULL; 	}
		| SELECT TT_SYMBOL stmt_config_selects
			{ $$ = YYTEST(next_token($2, $3, TK_LIST_EF_NULL)); 	}
;

stmt_config_type:
		  BOOL TT_BOOL
{
	if ($2.ttype != TT_BOOL)
		YYERROR;
	$$ = $2;
}
		| INTEGER TT_INTEGER
{
	if ($2.ttype != TT_INTEGER)
		YYERROR;
	$$ = $2;
}
		| STRING TT_DESCRIPTION
{
	if ($2.ttype != TT_DESCRIPTION)
		YYERROR;
	$$ = $2;
};

stmt_config: CONFIG stmt_config_description TT_SYMBOL
		stmt_config_type stmt_config_selects stmt_dependency stmt_help
{
	if ($3.ttype != TT_SYMBOL ||
		add_new_config_entry($2, $3,
			$4, $5, $6, $7) == -1)
		YYERROR;
};

/* ... THE 'CHOICE' SPECIFIC KEYWORD RULES. */

choice_int_ndef: 
		 /* optional: empty. */ 					{ $$ = NULL; 	}
		| OPTION TT_INTEGER choice_int_ndef
{
	$$ = YYTEST(next_token($2, $3,
		TK_LIST_EF_NULL));
};

choice_int_def:
		  OPTION TT_INTEGER choice_int_def 
{
	$$ = YYTEST(next_token($2, $3,
		TK_LIST_EF_NULL));
}
		| OPTION TT_INTEGER DEFAULT choice_int_ndef 
{
	$$ = YYTEST(next_token($2, $4,
		TK_LIST_EF_DEFAULT));
};

choice_str_ndef:
		 /* optional: empty. */ 					{ $$ = NULL; 	}
		| OPTION TT_DESCRIPTION choice_str_ndef 
{
	$$ = YYTEST(next_token($2, $3,
		TK_LIST_EF_NULL));
};

choice_str_def: 
		  OPTION TT_DESCRIPTION choice_str_def
{
	$$ = YYTEST(next_token($2, $3,
		TK_LIST_EF_NULL));
}
		| OPTION TT_DESCRIPTION DEFAULT choice_str_ndef
{
	$$ = YYTEST(next_token($2, $4,
		TK_LIST_EF_DEFAULT));
};

stmt_choice_options: /* ... options should have same type. */
		  choice_int_def | choice_str_def;

stmt_choice: CHOICE TT_DESCRIPTION TT_SYMBOL
		stmt_choice_options stmt_dependency stmt_help
{
	if ($2.ttype != TT_DESCRIPTION ||
		$3.ttype != TT_SYMBOL ||
		add_new_choice_entry($2, $3,
			$4, $5, $6) == -1)
		YYERROR;
};

%%

void yyerror(char *s) {
	fprintf(stderr, "error: %s\n", s);
}