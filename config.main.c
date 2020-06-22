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

#include <unistd.h>
#include <libgen.h>

#include "config.db.h"

extern int yy_parse_file(const char *filename);

void print_help(char *pname)
{
    printf("\nUse: %s [OPTIONS]\n", pname);
    printf("  [-C]      creates default '.old.config' from the input config file\n");
    printf("  [-u]      open the GUI\n");
    printf("  [-i file] choose input config file\n");
    printf("  [-o file] choose output autoconfig file\n");
}

#define _IN_FILE "configs.in"
#define _OUT_FILE "sys.config.h"
#define _IN_FOLDER dirname(in_filename)
int main(int argc, char *argv[])
{
    int option_index = 0;
    int gen_old_config = 0, need_gui = 0;
    _string_t in_filename = _IN_FILE, out_filename = _OUT_FILE;

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
    if (yy_parse_file(in_filename) != 0)
        return -1;

    if (chdir(_IN_FOLDER) == -1) {
        perror("Changing CWD.");
        return -1;
    }

    _token_list_t tp;
    while ((tp = config_files) != NULL) {
        _extended_token_t etoken = token_list_entry_info(tp);
        config_files = config_files->next; /* ... move next. */

        in_filename = strtok(etoken->token.TK_STRING, "\"");
        if (in_filename == NULL) {
            fprintf(stderr, "invalid '.include' in configuration file: %s\n",
                    etoken->token.TK_STRING);
            return -1;
        }

        if (yy_parse_file(in_filename) != 0)
            return -1;

        free(etoken);
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
            } else return 0;
        }

        if (build_autoconfig(out_filename) == -1) {
            perror("Building autoconfig:");
            return -1;
        }

        printf("Writing %s: Success\n", out_filename);
    }

    return 0;
}