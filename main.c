/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <search.h>

#include "db.h"
#include "defaults.h"

extern int start_gui(int);
extern int yy_parse_file(const char *filename);

static void print_help(char *pname)
{
    printf("\nUse: %s [OPTIONS]\n", pname);
    printf("  [--dump]             creates default '.old.config'\n");
    printf("  [--gui]              open the GUI\n");
    printf("  [--config file]      choose input config file\n");
    printf("  [--sys-config file]  choose output autoconfig file\n");
}

int gen_old_config = 0, need_gui = 0;

int main(int argc, char *argv[])
{
    struct include *file;
    string_t in_filename = _IN_FILE, out_filename = _OUT_FILE;

    while (1) {
        static struct option long_options[] = {
            {"dump", no_argument, &gen_old_config, 1},
            {"gui", no_argument, &need_gui, 1},
            {"config", required_argument, NULL, 'i'},
            {"sys-config", required_argument, NULL, 'o'},
            {"help", required_argument, NULL, 'h'}
        };

        int c = getopt_long(argc, argv, "", long_options, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 0:
            break;

        case 'i':
            in_filename = optarg;
            break;

        case 'o':
            out_filename = optarg;
            break;

        case 'h':
            print_help(argv[0]);
            return SUCCESS;

        case '?':
        default:
            print_help(argv[0]);
            return -1;
        }
    }

    hcreate(256);

    /* ... main configuration file. */
    if (yy_parse_file(in_filename) != 0)
        return -1;

    if (chdir(dirname(in_filename)) == -1) {
        perror("Changing CWD.");
        return -1;
    }

    LIST_FOREACH(file, &files, node) {
        curr_menu = file->menu;

        if (yy_parse_file(file->file) != 0)
            return -1;
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
            } else
                return SUCCESS;
        }

        if (build_autoconfig(out_filename) == -1) {
            perror("Building autoconfig:");
            return -1;
        }

        printf("Writing %s: Success\n", out_filename);
    }

    return SUCCESS;
}
