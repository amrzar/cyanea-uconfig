/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __NCURSES_GUI_H__
#define __NCURSES_GUI_H__

#include "db.h"
#include "defaults.h"

#define GUI_OPEN(_n, ...) open_ ## _n(int height, int width, int y, int x, __VA_ARGS__)

/* '__MAIN_MENU_HIGH' is size of the middle part of the screen, 2 lines for top
 * and bottom margins. 'MAIN_MENU_HIGH' is visible area in the middle of screen, 5 lines
 * is reserved form the bottom of the screen. */

#define __MAIN_MENU_HIGH (LINES - TITLE_HIGH - FOOTNOTE_HIGH - 2)
#define MAIN_MENU_HIGH (__MAIN_MENU_HIGH - 5)

#define SCREEN_WIDTH (COLS - 2 * MARGIN_LEFT)

#define POPUP_COLOR_ATTR COLOR_PAIR(1)
#define DISABLE_COLOR_ATTR COLOR_PAIR(2)

typedef const char *gkey_t;
enum kt {
    OK_CANCEL,
    EXIT,
    nr_keys
};

gkey_t get_keys(enum kt);

/*
 * 'wattrpr' prints string to a window, accepts following attributes:
 *
 * %b A_BOLD            : Enable extra bright or bold attribute
 * %u A_UNDERLINE       : Enable underline attribute
 * %H A_STANDOUT        : Activate best highlighting mode of the terminal
 * %h Reset A_STANDOUT  : Deactivate highlighting mode
 * %r Reset attribute   : Normal display
 *
 **/

static int wattrpr(WINDOW *win, int y, int x, const char *s)
{
    int i, attr = 0, slen = 0;
    wmove(win, y, x);

    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] == '%') {
            if (s[++i] != '\0') {
                if (s[i] == 'b')
                    attr |= A_BOLD;
                else if (s[i] == 'u')
                    attr |= A_UNDERLINE;
                else if (s[i] == 'r')
                    attr = 0;
                else if (s[i] == 'H')
                    wattron(win, A_STANDOUT);
                else if (s[i] == 'h')
                    wattroff(win, A_STANDOUT);
            } else
                break;

        } else {
            slen++;
            waddch(win, s[i] | attr);
        }
    }

    return slen;
}

extern int GUI_OPEN(pad, const char *, int);
extern int GUI_OPEN(file, const char *);
extern int GUI_OPEN(message_box, const char *, gkey_t);
extern string_t GUI_OPEN(input_box, const char *, const char *,
    const char *, const char *);
extern int GUI_OPEN(radio_box, const char *, string_t[], int, int, int);

#define input_box(help, prompt, str, regex) \
    open_input_box(MAIN_MENU_HIGH, SCREEN_WIDTH, TITLE_HIGH, MARGIN_LEFT, \
        (help), (prompt), (str), (regex))

static string_t int_input_box(const char *help, const char *prompt, integer_t i)
{
    string_t input;
    char tmp[64];

    switch (i.base) {
        case 16:
            snprintf(tmp, 64, "0x%X", i.n);
            input = input_box(help, prompt, tmp, "^0x[a-fA-F0-9]* *$");
            break;
        default:
            snprintf(tmp, 64, "%d", i.n);
            input = input_box(help, prompt, tmp, "^[0-9]* *$");
    }

    return input;
}

#define radio_box(help, choices, n, selected, size) \
    open_radio_box(MAIN_MENU_HIGH, SCREEN_WIDTH, TITLE_HIGH, MARGIN_LEFT, \
        (help), (choices), (n), (selected), (size))

#endif /* __NCURSES_GUI_H__ */
