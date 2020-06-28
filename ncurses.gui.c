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

#include <ncurses.h>

#include "ncurses.gui.h"
#include "y.tab.h"

static const char screen_title[] = {
    ".:%bCyanea ukernel%r Configuration:."
};

static const char *footnote_message[] = {
    "Press [%bReturn%r] to select and [%bBackspace%r] to go back ...",
    "Press [%bh%r] for help and [%bq%r] to exit.",
    NULL
};

enum config_type {
    CONF_MENU,
#define _CONF_MENU  "    [+] %s"
    CONF_YES,
#define _CONF_YES   "    [*] %s"
    CONF_NO,
#define _CONF_NO    "    [ ] %s"
    CONF_INPUT,
#define _CONF_INPUT "        %s"
    CONF_RADIO
#define _CONF_RADIO "        %s"
};

typedef struct {
    enum config_type t;
    void *private;
} config_t;

static WINDOW *main_screen;
static WINDOW *middle = NULL, *footnote = NULL;

static inline void __init_ncurses(void) {
    main_screen = initscr();

    curs_set(0);
    start_color();

    cbreak();
    noecho();
    set_escdelay(0);

    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);
}

static int init_screen(void) {
    static int X = 0, Y = 0;

    if (X != COLS || Y != LINES) {
        /* ... delete screen. */
        clear();
        delwin(middle);
        delwin(footnote);

        middle = subwin(main_screen, __MAIN_MENU_HIGH,
                SCREEN_WIDTH, TITLE_HIGH, MARGIN_LEFT);

        if (middle == NULL)
            return ERR;

        footnote = subwin(main_screen, FOOTNOTE_HIGH,
                SCREEN_WIDTH, LINES - FOOTNOTE_HIGH - 1, MARGIN_LEFT);

        if (footnote == NULL) {
            delwin(middle);
            return ERR;
        }

        getmaxyx(main_screen, Y, X);
    }

    return OK;
}

static int draw_screen(void) {
    if (init_screen() == ERR)
        return ERR;

    wborder(main_screen, 32, 32, 32, 32, 32, 32, 32, 32);
    wattrpr(main_screen, TITLE_LINE,
        COLS / 2 - strlen(screen_title) / 2, screen_title);

    for (int i = 0; i < FOOTNOTE_HIGH ||
        footnote_message[i] != NULL; i++)
        wattrpr(footnote, i, 0, footnote_message[i]);

    return OK;
}

#define PROMPT_MENU(_e) ((menu_t *)(_e).private)->prompt
#define PROMPT_ITEM(_e) ((item_t *)(_e).private)->common.prompt
static void draw_main_menu(const char *menu_title,
    config_t choices[], int current_highlight, int choice_start) {
    int menu_high = 0;
    int current_row = choice_start;

    draw_screen();
    werase(middle); /* ... erase previous menu. */
    wattron(middle, A_BOLD);
    mvwprintw(middle, 0, 0, "[-] %s", menu_title);
    wattroff(middle, A_BOLD);

    while (choices[current_row].private != NULL &&
        menu_high < MAIN_MENU_HIGH) {

        if (current_row == current_highlight)
            wattron(middle, A_STANDOUT);

        switch (choices[current_row].t) {
        case CONF_MENU:
            mvwprintw(middle, menu_high + 2, 0, _CONF_MENU,
                PROMPT_MENU(choices[current_row]));
            break;

        case CONF_YES:
            mvwprintw(middle, menu_high + 2, 0, _CONF_YES,
                PROMPT_ITEM(choices[current_row]));
            break;

        case CONF_NO:
            mvwprintw(middle, menu_high + 2, 0, _CONF_NO,
                PROMPT_ITEM(choices[current_row]));
            break;

        case CONF_INPUT:
            mvwprintw(middle, menu_high + 2, 0, _CONF_INPUT,
                PROMPT_ITEM(choices[current_row]));
            break;

        case CONF_RADIO:
            mvwprintw(middle, menu_high + 2, 0, _CONF_RADIO,
                PROMPT_ITEM(choices[current_row]));
            break;

        default: /* ... we never gets here! */
            mvwprintw(middle, menu_high + 2, 0,
                "err. unrecognised configuration item.");
        }

        if (current_row == current_highlight)
            wattroff(middle, A_STANDOUT);

        current_row++;
        menu_high++;
    }
}

#define SPECIAL_KEY_BS  -1  /* ... 'Backspace' pressed. */
#define SPECIAL_KEY_Q   -2  /* ... 'Q' pressed. */
#define SPECIAL_KEY_H   -3  /* ... 'H' pressed. */
#define SPECIAL_KEY_RT  -4  /* ... 'Retuen' pressed. */
#define SPECIAL_KEY_F1  -5  /* ... 'F1' pressed. */

static int __selected_row = 0;
static int main_menu_driver(const char *menu_title, config_t choices[]) {
    int choice_start = 0;
    int max_row = 0, getch_key = 0;

    while (choices[max_row].private != NULL)
        max_row++;

    if (__selected_row >= max_row)
        __selected_row = 0;

    keypad(main_screen, TRUE);

    while (getch_key != KEY_ENTER && getch_key != '\n') {
        if (COLS > TERMINAL_COLS && LINES > TERMINAL_LINES) {

            if (getch_key == KEY_UP) {
                if (__selected_row > 0)
                    __selected_row--;

                if (__selected_row < choice_start &&
                    choice_start > 0)
                    choice_start--;
            }

            if (getch_key == KEY_DOWN) {
                if (__selected_row < max_row - 1) {
                    __selected_row++;

                    if (__selected_row - choice_start >= MAIN_MENU_HIGH)
                        choice_start++;
                }
            }

            draw_main_menu(menu_title, choices, __selected_row, choice_start);

            /* ... process special keys. */
            if (getch_key == KEY_BACKSPACE)
                return SPECIAL_KEY_BS;

            if (getch_key == 'q')
                return SPECIAL_KEY_Q;

            if (getch_key == 'h')
                return SPECIAL_KEY_H;

            if (getch_key == KEY_F(1))
                return SPECIAL_KEY_F1;

        } else {
            clear();
            mvwprintw(main_screen, 0, 0, "%s",
                "Terminal is too small...");
        }

        /* ... also refresh 'main_screen', here. */
        getch_key = getch();
    }

    keypad(main_screen, FALSE);

    return SPECIAL_KEY_RT;
}

#define __realloc(_s, _n, _sz) ({                       \
        typeof(_s) tmp = realloc((_s), (_n) * (_sz));   \
        if (tmp == NULL)                                \
            free((_s));                                 \
        (tmp);                                          \
    })

#define relloc_conf(_c, _n) __realloc((_c), (_n), sizeof(config_t))
static config_t *menu_to_config_struct(menu_t *menu) {
    menu_t *m;
    item_t *item;

    int num = 1;
    static int conf_size = 0;
    static config_t *conf = NULL;

    list_for_each_entry(m, &menu->childs, sibling) {
        if (eval_expr(m->dependancy)) {

            if (++num > conf_size)
                if ((conf = relloc_conf(conf, num)) == NULL)
                    return NULL;

            conf[num - 2].private = m;
            conf[num - 2].t = CONF_MENU;
        }
    }

    list_for_each_entry(item, &menu->entries, list) {
        if (item->common.prompt != NULL &&
            eval_expr(item->common.dependancy)) {
            _extended_token_t etoken;

            if (++num > conf_size)
                if ((conf = relloc_conf(conf, num)) == NULL)
                    return NULL;

            conf[num - 2].private = item;

            etoken = item_token_list_head_entry(item);

            if (etoken->flags & TK_LIST_EF_CONFIG) {
                if (etoken->token.ttype == TT_BOOL)
                    conf[num - 2].t = etoken->token.TK_BOOL ?
                        CONF_YES : CONF_NO;
                else
                    conf[num - 2].t = CONF_INPUT;
            } else
                conf[num - 2].t = CONF_RADIO;
        }
    }

    /* ... set terminating entry. */
    conf[num - 1].private = NULL;

    if (num > conf_size)
        conf_size = num;

    return conf;
}

#define relloc_str(_c, _n) __realloc((_c), (_n), sizeof(_string_t))
static int __open_radio_item(item_t *item) {
    _token_list_t tp;

    int num = 0, in, selected = -1;
    _string_t *choices = NULL;

    item_token_list_for_each(tp, item) {
        _extended_token_t etoken = token_list_entry_info(tp);

        if ((choices = relloc_str(choices, ++num)) == NULL)
            return -1;

        if (etoken->token.ttype == TT_INTEGER) {
            choices[num - 1] = alloca(64);
            snprintf(choices[num - 1], 64, "%d", etoken->token.TK_INTEGER);

        } else /* and TT_DESCRIPTION. */
            choices[num - 1] = etoken->token.TK_STRING;

        if (etoken->flags & TK_LIST_EF_SELECTED)
            selected = num - 1;
    }

    in = RADIO_BOX("", choices, num, selected,
            (num > 5) ? 5 : num);

    if (in != -1)
        toggle_choice(item, choices[in]);

    free(choices);
    return 0;
}

#define cur_config config[__selected_row]
int start_gui(int nr_pages) {
    int k, ret = 0, index = 0;
    config_t *config;

    /* ... allocate stack for traversing menu. */
    menu_t **pages = calloc(nr_pages, sizeof(menu_t *));
    pages[index] = MAINMENU;

    __init_ncurses();

    while (1) {
        config = menu_to_config_struct(pages[index]);

        if (config == NULL) {
            ret = -2;
            break;
        }

        k = main_menu_driver((pages[index]->prompt == NULL) ?
                /* ... menu title: 'Options' as main title. */
                "Options" : pages[index]->prompt, config);

        switch (k) {
        case SPECIAL_KEY_BS:
            if (index > 0)
                index--;

            break;

        case SPECIAL_KEY_Q:
            while (1) {
                static const char exit_message[] = {
                    "%bAre you sure?%r"
                };

                static const char *exit_message_buttons[] = {
                    "[%bS%rave and Exit]",
                    "[E%bx%rit]",
                    "[%bC%rancel]",
                    NULL
                };

                int d = open_message_box(2, 90,
                        LINES / 2, COLS / 2 - 45,
                        exit_message, exit_message_buttons);

                if (d == 'x') {
                    ret = -1;
                    goto end_gui;
                }

                else if (d == 'S' ||
                    d == 's') {
                    goto end_gui;

                } else if (d == 'C' ||
                    d == 'c' ||
                    d == 27)
                    break;

                else if (d == KEY_RESIZE)
                    break; /* ... terminal resized. */
            }

            break;

        case SPECIAL_KEY_H:
            if (cur_config.t != CONF_MENU) {
                item_t *item = (item_t *)cur_config.private;

                __open_newpad(__MAIN_MENU_HIGH, SCREEN_WIDTH,
                    TITLE_HIGH, MARGIN_LEFT, (item->common.help == NULL) ?
                    "No help provided." : item->common.help, 150);
            }

            break;

        case SPECIAL_KEY_F1:
            open_textfile(__MAIN_MENU_HIGH, SCREEN_WIDTH,
                /* ... 'README.md' is inside the '_IN_FOLDER' folder. */
                TITLE_HIGH, MARGIN_LEFT, "README.md", 150);
            break;

        default: /* process 'SPECIAL_KEY_RT' */
            if (cur_config.t == CONF_MENU)
                pages[++index] = cur_config.private;

            else if (cur_config.t == CONF_YES)
                toggle_config(cur_config.private);

            else if (cur_config.t == CONF_NO)
                toggle_config(cur_config.private);

            else if (cur_config.t == CONF_INPUT) {
                item_t *item;
                _extended_token_t etoken;
                _string_t in;

                item = (item_t *)cur_config.private;
                etoken = item_token_list_head_entry(item);

                if (etoken->token.ttype == TT_INTEGER) {
                    char tmp[64];
                    snprintf (tmp, 64, "%d", etoken->token.TK_INTEGER);

                    /* ... regex: accept only numeric. */
                    in = INPUT_BOX("", item->common.prompt, tmp, "^[0-9]* *$");

                    if (in != NULL)
                        toggle_config(item, atoi(in));

                    free(in);
                } else { /* and TT_DESCRIPTION. */
                    in = INPUT_BOX("", item->common.prompt,
                            etoken->token.TK_STRING, "^\"[^\"]*\" *$");

                    if (in != NULL)
                        toggle_config(item, in);
                }
            } else /* and 'CONF_RADIO'. */
                __open_radio_item(cur_config.private);
        }
    }

end_gui:
    free(config);
    free(pages);

    clear();
    nocbreak();
    echo();
    endwin();

    return ret;
}