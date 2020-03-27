%{

#include "config.parser.h"
#include "config.db.h"
#include "y.tab.h"

extern void yyerror(char *s);

#define _IN_FILE "configs.in"
#define _OUT_FILE "sys.config.h"

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

int yywrap(void) { return 1; }

void print_help(char *pname) {
	printf("\nUse: %s [OPTIONS]\n", pname);
	printf("  [-C]      creates default '.old.config' from the input config file\n");
	printf("  [-i file] choose input config file; uses '%s' in current directory as default\n", _IN_FILE);
	printf("  [-o file] choose output autoconfig file; uses '%s' in current directory as default\n", _OUT_FILE);
}

const char *in_filename = _IN_FILE;
const char *out_filename = _OUT_FILE;

int main(int argc, char *argv[]) {
	FILE *filep;
	int option_index = 0;
	int gen_old_config = 0;
	int ret;

	while ((option_index = getopt(argc, argv, "i:o:Ch?")) != -1) {
		switch (option_index) {
			case 'i':
				in_filename = optarg;
				break;

			case 'o':
				out_filename = optarg;
				printf("Writing to %s\n", out_filename);
				break;

			case 'C':
				gen_old_config = 1;
				break;

			case 'h':
			case '?':
			default:
				print_help(argv[0]);
      			return -1;
		}
	}

	if ((filep = fopen(in_filename, "r")) == NULL) {
		perror("Unable to open config file:");
		return -1;
	}

	printf("... config file: %s\n", in_filename);
	yyset_in(filep);

	/* ... initialise symbol hash table. */
	init_symbol_hash_table();
	
	if ((ret = yyparse()) != 0) {
		fprintf(stderr, "yacc failed with %d\n", ret);
		goto done;
	}

	if (gen_old_config == 1) {
		if ((ret = create_config_file(".old.config")) == -1) {
			perror("Generateing '.old.config'");
			goto done;
		}

		printf("Generateing '.old.config': Success\n");
	} else {
		if ((ret = read_config_file(".old.config")) == -1) {
			perror("Opening '.old.config'");
			goto done;
		}

		/* ... open up GUI. */
		start_gui(25);

		if ((ret = build_autoconfig(out_filename)) == -1) {
			perror("Building autoconfig:");
			goto done;
		}

		printf("Writing '%s': Success\n", out_filename);
	}

done:	
	fclose(filep);
	return ret;
}