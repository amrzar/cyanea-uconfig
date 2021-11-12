%option noyywrap

%{

#include "config.parser.h"
#include "y.tab.h"

%}

ws				[ \t\n]+
comment			#.*
qstring			\"[^\"]*\"
symbol			[A-Z0-9][A-Z0-9_]+
integer			([0-9]|[1-9][0-9]+|0x[a-fA-F0-9]+)


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

{comment}	; /* ... ignore comments. */ ;
{ws} 		; /* ... ignore white spaces. */	;

(true|false) {
	yylval.token.TK_BOOL = 
		(strcmp(yytext, "true") == 0);
	yylval.token.ttype = TT_BOOL;
	return TT_BOOL;
}

{integer} {
	yylval.token.TK_INTEGER = strtol(yytext, NULL, 0);
	yylval.token.info.number.base = strncmp(yytext, "0x", 2) == 0 ? 16 : 10;
	yylval.token.ttype = TT_INTEGER;
	return TT_INTEGER;
}

{symbol} {
	yylval.token.TK_STRING = strdup(yytext);
	yylval.token.ttype = TT_SYMBOL;
	return TT_SYMBOL;
}

{qstring} {
	yylval.token.TK_STRING = strdup(yytext + 1); /* skip open quote  */
	yylval.token.TK_STRING[yyleng - 2] = '\0'; /* remove close quote */
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
    int ret = yyparse();

    fclose(filep);
    return ret;
}
