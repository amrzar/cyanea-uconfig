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

#include "config.db.h"
#include "y.tab.h"

static const char TITLE[] = {
    ".:%bCyanea ukernel%r Configuration:."
};

static const char *FOOTNOTE[] = {
    "Press [%bReturn%r] to select and [%bBackspace%r] to go back ...",
    "Press [%bh%r] for help and [%bq%r] to exit.",
    NULL
};

static const char exit_message[] = {
    "All changes are written to '.old.config' and system configuration header is generated.\n" \
    "... to ignore all changes choose exit."
};

static const char *exit_message_buttons[] = {
    "%H[%bS%rave and Exit]%h",
    "%H[E%bx%rit]%h",
    "%H[%bC%rancel]%h",
    NULL
};

#define TITLE_LINE 2	/* ... index of screen's title line. */
#define TITLE_HIGH 6	/* ... number of lines in title section. */
#define FOOTNOTE_HIGH 2	/* ... number of lines in footnote section. */
#define MARGIN_LEFT 2	/* ... index of sreen's margin. */

#define MAIN_MENU_HIGH LINES - TITLE_HIGH - FOOTNOTE_HIGH - 2
/* ... size of menu on screen: visible area in the middle of screen. */
#define VISIBLE_MAIN_MENU_HIGH MAIN_MENU_HIGH - 5
#define SCREEN_WIDTH COLS - 2 * MARGIN_LEFT

/* 'MIN_TERM_LINES x MIN_TERM_COLS' is minimum size of terminal. */
#define MIN_TERM_LINES 30
#define MIN_TERM_COLS 80

extern int wattrp(WINDOW *, int, int, const char *);
extern int open_textfile(int, int, int, int, const char *, int);
extern int open_message_box(int, int, int, int, const char *, const char *[]);
extern _string_t open_input_box(int, int, int, int,
                                const char *, const char *, _string_t, const char *);
extern int open_radio_box(int, int, int, int,
                          const char *, _string_t [], int, int, int);

enum config_type {
    CONF_MENU,
#define _CONF_MENU 	"[+] %s"
    CONF_YES,
#define _CONF_YES 	"[*] %s"
    CONF_NO,
#define _CONF_NO 	"[ ] %s"
    CONF_INPUT,
#define _CONF_INPUT "    %s"
    CONF_RADIO
#define _CONF_RADIO "    %s"
};

typedef struct {
    enum config_type t;
    void *__private;
} config_t;

static WINDOW *__main_screen;
/* ... secreen sections: middle and footnote. */
static WINDOW *middle = NULL, *fnote = NULL;

static int init_screen(void)
{
    static int X = 0, Y = 0;

    if (X != COLS || Y != LINES) {
        /* ... delete screen. */
        clear();
        delwin(middle);
        delwin(fnote);

        if ((middle = subwin(__main_screen,
                             MAIN_MENU_HIGH, SCREEN_WIDTH,
                             TITLE_HIGH, MARGIN_LEFT)) == NULL)
            return ERR;

        if ((fnote = subwin(__main_screen,
                            FOOTNOTE_HIGH, SCREEN_WIDTH,
                            LINES - FOOTNOTE_HIGH - 1, MARGIN_LEFT)) == NULL) {
            delwin(middle);
            return ERR;
        }

        getmaxyx(__main_screen, Y, X);
    }
    return OK;
}

static int draw_screen(void)
{
    int line, xtitle = COLS / 2 - strlen(TITLE) / 2;

    /* ... partition the screen. */
    if (init_screen() == ERR)
        return ERR;

    /* ... assume a place holder for screen border: use space. */
    wborder(__main_screen, 32, 32, 32, 32, 32, 32, 32, 32);
    wattrp(__main_screen, TITLE_LINE, xtitle, TITLE);

    for (line = 0; line < FOOTNOTE_HIGH ||
         FOOTNOTE[line] != NULL; line++) {
        /* ... make sure text fit the footnote. */
        if (strlen(FOOTNOTE[line]) < SCREEN_WIDTH)
            wattrp(fnote, line, 0, FOOTNOTE[line]);
    }

    return OK;
}

#define PROMPT_MENU(_e) ((menu_t *)(_e).__private)->prompt
#define PROMPT_ITEM(_e) ((item_t *)(_e).__private)->common.prompt
static void draw_main_menu(const char *menu_title,
                           config_t choices[], int current_highlight, int choice_start)
{
    int menu_high = 0;
    int current_row = choice_start;

    draw_screen();
    werase(middle); /* ... erase previous menu. */
    wattron(middle, A_BOLD);
    mvwprintw(middle, 0, 0, "[-] %s", menu_title);
    wattroff(middle, A_BOLD);

    while (choices[current_row].__private != NULL &&
           menu_high < VISIBLE_MAIN_MENU_HIGH) {

        if (current_row == current_highlight)
            wattron(middle, A_STANDOUT);

        switch (choices[current_row].t) {
        case CONF_MENU:
            mvwprintw(middle, menu_high + 2, 0, _CONF_MENU,
                      PROMPT_MENU(choices[current_row]));
            break;
        case CONF_YES:
            mvwprintw(middle, menu_high + 2, 0,	_CONF_YES,
                      PROMPT_ITEM(choices[current_row]));
            break;
        case CONF_NO:
            mvwprintw(middle, menu_high + 2, 0,	_CONF_NO,
                      PROMPT_ITEM(choices[current_row]));
            break;
        case CONF_INPUT:
            mvwprintw(middle, menu_high + 2, 0, _CONF_INPUT,
                      PROMPT_ITEM(choices[current_row]));
            break;
        case CONF_RADIO:
            mvwprintw(middle, menu_high + 2, 0,	_CONF_RADIO,
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

/*
 * 'main_menu_driver' is the main entry to the GUI.
 * It returns the special key pressed or the index in to the menu. */

#define SPECIAL_KEY_BS	-1	/* ... 'Backspace' pressed. */
#define SPECIAL_KEY_Q	-2	/* ... 'Q' pressed. */
#define SPECIAL_KEY_H	-3	/* ... 'H' pressed. */
static int main_menu_driver(const char *menu_title,
                            config_t choices[], int selected_row)
{
    int choice_start = 0;
    int max_row = 0, getch_key = 0;

    while (choices[max_row].__private != NULL)
        max_row++;

    if (selected_row >= max_row)
        selected_row = 0;

    keypad(__main_screen, TRUE);

    while (getch_key != KEY_ENTER && getch_key != '\n') {
        if (COLS > MIN_TERM_COLS && LINES > MIN_TERM_LINES) {

            if (getch_key == KEY_UP) {
                if (selected_row > 0)
                    selected_row--;
                if (selected_row < choice_start &&
                    choice_start > 0)
                    choice_start--;
            }
            if (getch_key == KEY_DOWN) {
                if (selected_row < max_row - 1) {
                    selected_row++;
                    if (selected_row - choice_start >= VISIBLE_MAIN_MENU_HIGH)
                        choice_start++;
                }
            }

            draw_main_menu(menu_title, choices, selected_row, choice_start);

            /* ... process special keys. */
            if (getch_key == KEY_BACKSPACE)
                return SPECIAL_KEY_BS;

            if (getch_key == 'q')
                return SPECIAL_KEY_Q;

            if (getch_key == 'h')
                return SPECIAL_KEY_H;

        } else {
            clear();
            mvwprintw(__main_screen, 0, 0, "%s",
                      "Terminal is too small...");
        }

        /* ... also refresh '__main_screen', here. */
        getch_key = getch();
    }

    keypad(__main_screen, FALSE);

    return selected_row;
}

static config_t *__construct_page(menu_t *menu)
{
    menu_t *m;
    item_t *item;

    int index = 0;
    config_t *tmp, *conf;

    if ((conf = malloc(sizeof(config_t))) == NULL)
        return NULL;

    list_for_each_entry(m, &menu->childs, sibling) {
        if (eval_expr(m->dependancy)) {
            index++; /* ... new entry. */
            if ((tmp = realloc(conf,
                               (index + 1) * sizeof(config_t))) == NULL) {
                free(conf);
                return NULL;
            }

            conf = tmp;
            conf[index - 1].t = CONF_MENU;
            conf[index - 1].__private = m;
        }
    }

    list_for_each_entry(item, &menu->entries, list) {
        _extended_token_t etoken;

        if (item->common.prompt != NULL &&
            eval_expr(item->common.dependancy)) {

            /* ... never is NULL. */
            etoken = item_token_list_head_entry(item);

            index++;
            if ((tmp = realloc(conf,
                               (index + 1) * sizeof(config_t))) == NULL) {
                free(conf);
                return NULL;
            }

            conf = tmp;
            conf[index - 1].__private = item;
            if (etoken->flags & TK_LIST_EF_CONFIG) {
                if (etoken->token.ttype == TT_BOOL)
                    conf[index - 1].t = etoken->token.TK_BOOL ?
                                        CONF_YES : CONF_NO;
                else
                    conf[index - 1].t = CONF_INPUT;
            } else
                conf[index - 1].t = CONF_RADIO;
        }
    }

    /* ... set terminating entry. */
    conf[index].__private = NULL;
    return conf;
}

static int radio_item(item_t *item)
{
    _token_list_t tp;

    int i = 0, in, selected = -1;
    char **choices = NULL;
    item_token_list_for_each(tp, item) {
        _extended_token_t etoken = token_list_entry_info(tp);

        _string_t *tmp;
        if ((tmp = realloc(choices,
                           (i + 1) * sizeof(_string_t))) == NULL) {
            free(choices);
            return -1;
        }

        choices = tmp;
        if (etoken->token.ttype == TT_INTEGER) {
            choices[i] = alloca(64);
            sprintf(choices[i], "%d", etoken->token.TK_INTEGER);

        } else /* and TT_DESCRIPTION. */
            choices[i] = etoken->token.TK_STRING;

        if (etoken->flags & TK_LIST_EF_SELECTED)
            selected = i;

        i++;
    }

    if ((in = open_radio_box(VISIBLE_MAIN_MENU_HIGH, SCREEN_WIDTH,
                             TITLE_HIGH, MARGIN_LEFT, "", choices, i, selected, 5)) != -1)
        toggle_choice(item, choices[in]);

    return 0;
}

int start_gui(int nr_pages)
{
    int k, ret = 0;
    int choice = 0, index = 0;

    config_t *curr_config;
    menu_t **pages = calloc(nr_pages, sizeof(menu_t *));
    pages[index] = &mainmenu;

    __main_screen = initscr();

    curs_set(0);
    start_color();

    cbreak();
    noecho();

    init_pair(1, COLOR_WHITE, COLOR_BLUE); /* ... popup window. */
    init_pair(2, COLOR_BLACK, COLOR_BLUE); /* ... disable. */

    while (1) {
        curr_config = __construct_page(pages[index]);
        k = main_menu_driver((pages[index]->prompt == NULL) ?

                             /* ... menu title: 'Options' as main title. */
                             "Options" : pages[index]->prompt,
                             curr_config, choice);

        switch (k) {
        case SPECIAL_KEY_BS:
            if (index > 0) index--;
            break;

        case SPECIAL_KEY_Q:
            while(1) {
                int d = open_message_box(2, 90, LINES / 2,
                                         COLS / 2 - 45, exit_message, exit_message_buttons);

                if (d == 'x') {
                    ret = -1;
                    goto end_gui;
                }

                else if (d == 'S' || d == 's') {
                    goto end_gui;

                } else if (d == 'C' || d == 'c')
                    break;

                else if (d == KEY_RESIZE)
                    break; /* ... terminal resized. */
            }
            break;

        case SPECIAL_KEY_H:
            open_textfile(MAIN_MENU_HIGH, SCREEN_WIDTH,
                          TITLE_HIGH, MARGIN_LEFT, "README.md", 150);
            break;

        default:
            choice = k; /* ... 'k' is not special key. */
            if (curr_config[choice].t == CONF_MENU)
                pages[++index] = curr_config[choice].__private;

            else if (curr_config[choice].t == CONF_YES)
                toggle_config(curr_config[choice].__private);

            else if (curr_config[choice].t == CONF_NO)
                toggle_config(curr_config[choice].__private);

            else if (curr_config[choice].t == CONF_INPUT) {
                _string_t in;
                item_t *item = (item_t *)curr_config[choice].__private;

                _extended_token_t etoken = item_token_list_head_entry(item);
                if (etoken->token.ttype == TT_INTEGER) {
                    char tmp[64];
                    sprintf (tmp, "%d", etoken->token.TK_INTEGER);

                    in = open_input_box(VISIBLE_MAIN_MENU_HIGH, SCREEN_WIDTH,
                                        TITLE_HIGH, MARGIN_LEFT, "", item->common.prompt,
                                        tmp, "^[0-9]* *$"); /* ... regex: accept only numeric. */

                    if (in != NULL)
                        toggle_config(item, atoi(in));

                } else { /* and TT_DESCRIPTION. */
                    in = open_input_box(VISIBLE_MAIN_MENU_HIGH, SCREEN_WIDTH,
                                        TITLE_HIGH, MARGIN_LEFT, "", item->common.prompt,
                                        etoken->token.TK_STRING, "^\"[^\"]*\" *$"); /* ... regex: description. */

                    if (in != NULL)
                        toggle_config(item, in);
                }
            } else /* and 'CONF_RADIO'. */
                radio_item(curr_config[choice].__private);
        }

        free(curr_config);
    }

end_gui:
    clear();
    nocbreak();
    echo();
    endwin();

    free(pages);

    return ret;
}