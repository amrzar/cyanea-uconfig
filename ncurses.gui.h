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
#include "defaults.h"

#define GUI_FUNCTION(_n, ...) _n(int height, int width, int y, int x, __VA_ARGS__)

/* '__MAIN_MENU_HIGH' is size of the middle part of the screen, 2 lines for top
 * and bottom margins. 'MAIN_MENU_HIGH' is visible area in the middle of screen, 5 lines
 * is reserved form the bottom of the screen. */

#define __MAIN_MENU_HIGH (LINES - TITLE_HIGH - FOOTNOTE_HIGH - 2)
#define MAIN_MENU_HIGH (__MAIN_MENU_HIGH - 5)

#define SCREEN_WIDTH (COLS - 2 * MARGIN_LEFT)

#define POPUP_COLOR_ATTR COLOR_PAIR(1)
#define DISABLE_COLOR_ATTR COLOR_PAIR(2)

typedef const char **_key_t;

/* 'wattrpr' prints string to a window, accepts following attributes:
 *
 * %b A_BOLD            : Enable extra bright or bold attribute
 * %u A_UNDERLINE       : Enable underline attribute
 * %H A_STANDOUT        : Activate best highlighting mode of the terminal
 * %h Reset A_STANDOUT  : Deactivate highlighting mode
 * %r Reset attribute   : Normal display
 *
 * 'open_newpad' creates a scrollable window.
 * 'wattrpr' and 'open_newpad' retuen ERR on failure and OK on success. */

extern int wattrpr(WINDOW *, int, int, const char *);
extern int GUI_FUNCTION(open_newpad, const char *, int);

extern int GUI_FUNCTION(open_textfile, const char *);
extern int GUI_FUNCTION(open_message_box, const char *, _key_t);
extern _string_t GUI_FUNCTION(open_input_box, const char *, const char *, const char *, const char *);
extern int GUI_FUNCTION(open_radio_box, const char *, _string_t[], int, int, int);

#define screen_input_box(_h, _p, _d, _r)                                    \
    open_input_box(MAIN_MENU_HIGH, SCREEN_WIDTH, TITLE_HIGH, MARGIN_LEFT,   \
        (_h), (_p), (_d), (_r))

#define screen_radio_box(_h, _c, _n, _s, _size)                             \
    open_radio_box(MAIN_MENU_HIGH, SCREEN_WIDTH, TITLE_HIGH, MARGIN_LEFT,   \
        (_h), (_c), (_n), (_s), (_size))

#endif /* __NCURSES_GUI_H__ */