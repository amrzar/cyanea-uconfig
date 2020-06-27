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

#include <ctype.h>
#include <ncurses.h>
#include <form.h>
#include <menu.h>

#include "ncurses.gui.h"

static inline _string_t trim_tail(_string_t str) {
    int slen = strlen(str);

    while (isspace(str[slen - 1]) != 0)
        slen--;

    return strndup(str, slen);
}

int wattrpr(WINDOW *win, int y, int x, const char *s) {
    int attr = 0, slen = 0;
    wmove(win, y, x);

    for (int i = 0; i < strlen(s); i++) {
        if (s[i] == '%') {
            if (s[++i] == 'b')
                attr |= A_BOLD;
            else if (s[i] == 'u')
                attr |= A_UNDERLINE;
            else if (s[i] == 'r')
                attr = 0;
            else if (s[i] == 'H')
                wattron(win, A_STANDOUT);
            else if (s[i] == 'h')
                wattroff(win, A_STANDOUT);
        } else {
            slen++;
            waddch(win, s[i] | attr);
        }
    }

    return slen;
}

int open_textfile(int height, int width,
    int y, int x, const char *filename, int ssize) {
    WINDOW *pad;

    _string_t line = NULL;
    size_t n = 0;
    int getch_key, pad_row = 0;

    /* ... pad with 'ssize' area. */
    if ((pad = newpad(ssize, width)) == NULL)
        return ERR;

    FILE *fp = fopen(filename, "r");

    if (fp != NULL) {
        while (getline(&line, &n, fp) != -1)
            wprintw(pad, "%s", line);

        fclose(fp);

        if (line != NULL)
            free(line);
    } else
        wprintw(pad, "Err. '%s'\n", filename);

    keypad(pad, TRUE);

    EVENTLOOP {
        prefresh(pad, pad_row, 0,
            y, x, height + y - 1, width + x - 1);

        getch_key = wgetch(pad);

        if (getch_key == KEY_UP) {
            if (pad_row > 0)
                pad_row--;
        } else if (getch_key == KEY_DOWN) {
            if (pad_row < ssize - height)
                pad_row++;
        } else
            break;
    }

    delwin(pad);
    return OK;
}

static WINDOW *__popup, *__message;
#define POPUP __popup
#define MESSAGE __message
static int newpopup(int height, int width,
    int y, int x, int H, const char *message, const char *keys[]) {
    int offset, i;

    POPUP = newwin(height + 4, width + 2, y - 1, x - 1);

    if (POPUP == NULL)
        return ERR;

    wbkgd(POPUP, POPUP_COLOR_ATTR);
    wborder(POPUP, 0, 0, 0, 0, 0, 0, 0, 0);

    MESSAGE = derwin(POPUP, height - H, width - 2, 1, 2);

    if (MESSAGE == NULL) {
        delwin(POPUP);
        return ERR;
    }

    wbkgd(MESSAGE, POPUP_COLOR_ATTR);
    wattrpr(MESSAGE, 0, 0, message);

    for (i = 0, offset = 2; keys[i] != NULL; i++, offset++)
        offset += wattrpr(POPUP, height + 2, offset, keys[i]);

    return OK;
}

static int delpopup(void) {
    if (delwin(__popup) == ERR ||
        delwin(__message) == ERR)
        return ERR;

    return OK;
}

#define __getchar(_k) do {      \
        (_k) = wgetch(POPUP);   \
    } while(0)

int open_message_box(int height, int width,
    int y, int x, const char *message, const char *keys[]) {
    int getch_key;

    if (newpopup(height, width, y, x, 0, message, keys) == ERR)
        return ERR;

    keypad(POPUP, TRUE);
    __getchar(getch_key);
    delpopup();

    return getch_key;
}

static const char **okcancel_keys(void) {
    static const char *keys[] = {
        "[%bReturn%r]"
        "[%bEsc%r]",
        NULL
    };

    return keys;
}

_string_t open_input_box(int height, int width,
    int y, int x, const char *help, const char *message,
    _string_t initial, const char *regexp) {

    int getch_key;
    WINDOW *form_win;

    _string_t ret = NULL;

    if (newpopup(height, width, y, x, 1, help, okcancel_keys()) == ERR)
        return NULL;

    if ((form_win = derwin(POPUP, 1, width - 2, height, 2)) == NULL)
        goto input_box_exit;

    FIELD *fields[3] = { NULL };
#define __caption fields[0]
#define __textbox fields[1]

    if ((__caption = new_field(1, strlen(message), 0, 0, 0, 0)) == NULL)
        goto input_box_exit;

    if ((__textbox = new_field(1, width - strlen(message) - 3,
                    0, strlen(message) + 1, 0, 0)) == NULL) {
        free_field(__caption);
        goto input_box_exit;
    }

#define __init_field(_w, _m, _a, _o) do {   \
        set_field_buffer((_w), 0, (_m));        \
        set_field_opts((_w), (_o));             \
        set_field_back((_w), (_a));             \
    } while (0)

    __init_field(__caption, message, POPUP_COLOR_ATTR,
        O_STATIC | O_VISIBLE | O_PUBLIC | O_AUTOSKIP);

    __init_field(__textbox, initial, A_BOLD,
        O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
    set_field_type(__textbox, TYPE_REGEXP, regexp);

    FORM *form;

    if ((form = new_form(fields)) == NULL)
        goto input_box_exit_form_field;

    set_form_win(form, POPUP);
    set_form_sub(form, form_win);
    post_form(form);

    curs_set(1);
    keypad(POPUP, TRUE);

    EVENTLOOP {
        __getchar(getch_key);

        if (getch_key == KEY_RESIZE ||
            getch_key == 27)
            break;

        if (getch_key == KEY_ENTER ||
            getch_key == '\n') {
            if (form_driver(form, REQ_VALIDATION) != E_OK)
                continue;

            ret = trim_tail(field_buffer(__textbox, 0));
            break;
        }

        switch (getch_key) {
        case KEY_LEFT:
            form_driver(form, REQ_PREV_CHAR);
            break;

        case KEY_RIGHT:
            form_driver(form, REQ_NEXT_CHAR);
            break;

        case KEY_BACKSPACE:
        case 127:
            form_driver(form, REQ_DEL_PREV);
            break;

        case KEY_DC:
            form_driver(form, REQ_DEL_CHAR);
            break;

        default:
            form_driver(form, getch_key);
            break;
        }
    }

    curs_set(0);
    unpost_form(form);
    free_form(form);

input_box_exit_form_field:
    free_field(__caption);
    free_field(__textbox);

input_box_exit:
    delwin(form_win);
    delpopup();
    return ret;
}

int open_radio_box(int height, int width, int y, int x,
    const char *message, _string_t choices[],
    int max_row, int selected, int ssize) {

    WINDOW *menu_win;
    int getch_key, i;

    int ret = -1;

    if (newpopup(height, width, y, x, ssize, message, okcancel_keys()) == ERR)
        return -1;

    if ((menu_win = derwin(POPUP, ssize, width - 2,
                    height - ssize + 1, 2)) == NULL)
        goto radio_box_exit;

    ITEM **items = alloca((max_row + 1) * sizeof(ITEM *));

    for (i = 0; i < max_row; i++) {
        items[i] = new_item(choices[i],
                (i == selected) ? "[*]" : "[ ]");

        if (items[i] == NULL)
            goto radio_box_exit_from_item;
    }

    items[i] = NULL;

    MENU *menu;

    if ((menu = new_menu(items)) == NULL)
        goto radio_box_exit_from_item;

    set_menu_win(menu, POPUP);
    set_menu_sub(menu, menu_win);
    set_menu_mark(menu, "  ");
    set_menu_format(menu, ssize, 1);
    set_menu_back(menu, POPUP_COLOR_ATTR);
    post_menu(menu);

    keypad(POPUP, TRUE);

    EVENTLOOP {
        __getchar(getch_key);

        if (getch_key == KEY_RESIZE ||
            getch_key == 27)
            break;

        if (getch_key == KEY_ENTER ||
            getch_key == '\n') {
            ret = item_index(current_item(menu));
            break;
        }

        switch (getch_key) {
        case KEY_DOWN:
            menu_driver(menu, REQ_DOWN_ITEM);
            break;

        case KEY_UP:
            menu_driver(menu, REQ_UP_ITEM);
            break;

        case KEY_NPAGE:
            menu_driver(menu, REQ_SCR_DPAGE);
            break;

        case KEY_PPAGE:
            menu_driver(menu, REQ_SCR_UPAGE);
            break;
        }
    }

    unpost_menu(menu);
    free_menu(menu);

radio_box_exit_from_item:

    while (i > 0)
        free_item(items[--i]);

radio_box_exit:
    delwin(menu_win);
    delpopup();
    return ret;
}
