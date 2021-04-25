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

#ifndef __NCURSES_GUI_H__
#define __NCURSES_GUI_H__

#include "config.db.h"

#define TERMINAL_LINES 30
#define TERMINAL_COLS 100

#define TITLE_LINE 2    /* ... index of screen's title line. */
#define TITLE_HIGH 6    /* ... number of lines in title section. */
#define FOOTNOTE_HIGH 2 /* ... number of lines in footnote section. */
#define MARGIN_LEFT 2   /* ... index of screen's margin. */

#define __MAIN_MENU_HIGH (LINES - TITLE_HIGH - FOOTNOTE_HIGH - 2)

/* ... size of menu on screen: visible area in the MIDDLE of screen. */
#define MAIN_MENU_HIGH (__MAIN_MENU_HIGH - 5)
#define SCREEN_WIDTH (COLS - 2 * MARGIN_LEFT)

#define POPUP_COLOR_ATTR COLOR_PAIR(1)
#define DISABLE_COLOR_ATTR COLOR_PAIR(2)

extern int wattrpr(WINDOW *, int, int, const char *);
extern int __open_newpad(int, int, int, int, _string_t, int);
extern int open_textfile(int, int, int, int, const char *, int);
extern int open_message_box(int, int, int, int, const char *, const char *[]);
extern _string_t open_input_box(int, int, int, int, const char *, const char *, _string_t, const char *);
extern int open_radio_box(int, int, int, int, const char *, _string_t[], int, int, int);

#define INPUT_BOX(h, prompt, def, regex)        \
    open_input_box(MAIN_MENU_HIGH,              \
        SCREEN_WIDTH, TITLE_HIGH, MARGIN_LEFT,  \
        h, prompt, def, regex)

#define RADIO_BOX(h, choices, n, sel, size)     \
    open_radio_box(MAIN_MENU_HIGH,              \
        SCREEN_WIDTH, TITLE_HIGH, MARGIN_LEFT,  \
        h, choices, n, sel, size)

#endif /* __NCURSES_GUI_H__ */