%{

#include <unistd.h>
#include <libgen.h>

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

int yywrap(void) { return 1; }

void print_help(char *pname) {
	printf("\nUse: %s [OPTIONS]\n", pname);
	printf("  [-C]      creates default '.old.config' from the input config file\n");
	printf("  [-u]      open the GUI\n");
	printf("  [-i file] choose input config file; uses '%s' in current directory as default\n", _IN_FILE);
	printf("  [-o file] choose output autoconfig file; uses '%s' in current directory as default\n", _OUT_FILE);
}

/* ... default configuration and autoconfig files. */
char *in_filename = _IN_FILE, *out_filename = _OUT_FILE;

static inline int parse_file(const char *filename) {
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

int main(int argc, char *argv[]) {

	int option_index = 0;
	int gen_old_config = 0, need_gui = 0;

	while ((option_index = getopt(argc, argv, "i:o:Ch?u")) != -1) {
		switch (option_index) {
			case 'i':
				in_filename = optarg;
				break;

			case 'o':
				out_filename = optarg;
				break;

			case 'C':
				gen_old_config = 1;
				break;

			case 'u':
				need_gui = 1;
				break;

			case 'h':
			case '?':
			default:
				print_help(argv[0]);
      			return -1;
		}
	}

	/* ... initialise symbol hash table. */
	init_symbol_hash_table();

	/* ... main configuration file. */
	if (parse_file(in_filename) != 0)
		return -1;

	if (chdir(dirname(in_filename)) == -1) {
		perror("Changing cwd");
		return -1;
	}

	_token_list_t tp;
	while ((tp = config_files) != NULL) {
		_extended_token_t etoken = token_list_entry_info(tp);
		config_files = config_files->next; /* ... move next. */

		in_filename = dupprompt(etoken->token.TK_STRING);
		if (in_filename == NULL) {
			fprintf(stderr, "invalid '.include' in configuration file: %s\n",
				etoken->token.TK_STRING);
			return -1;
		}

		if (parse_file(in_filename) != 0)
			return -1;

		free(in_filename);
		free(etoken);
		free(tp);
	}

	if (gen_old_config == 1) {
		if (create_config_file(".old.config") == -1) {
			perror("Generateing '.old.config'");
			return -1;
		}

		printf("Generateing '.old.config': Success\n");
	} else {
		if (read_config_file(".old.config") == -1) {
			perror("Opening '.old.config'");
			return -1;
		}

		/* ... open up GUI: 25 pages. */
		if (need_gui == 1) {
			if (start_gui(25) == 0) {
				if (write_config_file(".old.config") == -1) {
					perror("Writing '.old.config'");
					return -1;
				}
			}
			else return 0;
		}

		if (build_autoconfig(out_filename) == -1) {
			perror("Building autoconfig:");
			return -1;
		}

		printf("Writing %s: Success\n", out_filename);
	}

	return 0;
}