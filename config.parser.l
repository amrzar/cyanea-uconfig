%option noyywrap

%{

#include "config.parser.h"
#include "y.tab.h"

extern void yyerror(char *s);

%}

%%

"menu"			{ return MENU; }
"endmenu"		{ return END; }
"BOOL"			{ return BOOL; }
"INTEGER"		{ return INTEGER; }
"STRING"		{ return STRING; }
"help"			{ return HELP; }
"depends"		{ return DEPENDES; }
"config"		{ return CONFIG; }
"choice"		{ return CHOICE; }
"option"		{ return OPTION; }
"select"		{ return SELECT; }
".include"		{ return INCLUDE; }
"[default]"		{ return DEFAULT; }

\(				{ return OPENPAREN; }
\)				{ return CLOSEPAREN; }

"&&"			{ return AND; }
"||"			{ return OR; }
"NOT"			{ return NOT; }
"=="			{ return EQUAL; }
"!="			{ return NEQUAL; }

#.* /* ... ignore comments. */ ;
[ \t\n]+ /* ... ignore white spaces. */	;

(true|false) {
	yylval.token.TK_BOOL = 
		(strcmp(yytext, "true") == 0);
	yylval.token.ttype = TT_BOOL;
	return TT_BOOL;
}

[1-9][0-9]+ {
	yylval.token.TK_INTEGER = atoi(yytext);
	yylval.token.ttype = TT_INTEGER;
	return TT_INTEGER;
}

[0-9] {
	yylval.token.TK_INTEGER = atoi(yytext);
	yylval.token.ttype = TT_INTEGER;
	return TT_INTEGER;
}

[A-Z0-9][A-Z0-9_]+ {
	yylval.token.TK_STRING = strdup(yytext);
	yylval.token.ttype = TT_SYMBOL;
	return TT_SYMBOL;
}

["]([^".]|[\.\n])*["] {
	yylval.token.TK_STRING = strdup(yytext);
	yylval.token.ttype = TT_DESCRIPTION;
	return TT_DESCRIPTION;
}

. {
	return TT_INVALID; 
}

%%

int yy_parse_file(const char *filename) {
    FILE *filep;

    printf("... config file: %s\n", filename);

    if ((filep = fopen(filename, "r")) == NULL) {
        perror("Unable to open config file");
        return -1;
    }

    yyset_in(filep);

    int ret = 0;

    if ((ret = yyparse()) != 0)
        fprintf(stderr, "yacc failed with %d\n", ret);

    fclose(filep);

    return ret;
}
