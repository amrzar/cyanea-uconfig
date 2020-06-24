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

#include "config.db.h"

int wattrpr(WINDOW *win, int y, int x, const char *s)
{
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

static WINDOW *draw_popup(int height, int width,
                          int y, int x, const char *keys[])
{
    int offset = 2, i;
    WINDOW *wpopup;

    /* ... only assume two lines for buttons plus borsers. */
    if ((wpopup = newwin(height + 4, width + 2, y - 1, x - 1)) != NULL) {
        wbkgd(wpopup, COLOR_PAIR(1));
        wborder(wpopup, 0, 0, 0, 0, 0, 0, 0, 0);

        for (i = 0; keys[i] != NULL; i++) {
            offset += wattrpr(wpopup,
                              height + 2, offset, keys[i]);
            offset++;
        }
    }

    return wpopup;
}

int open_textfile(int height, int width,
                  int y, int x, const char *filename, int ssize)
{
    WINDOW *pad;

    _string_t line = NULL;
    size_t n = 0;
    int getch_key, pad_row = 0;

    /* ... pad with 'ssize' area. */
    if ((pad = newpad(ssize, width)) == NULL)
        return ERR;

    FILE *fp;
    if ((fp = fopen(filename, "r")) != NULL) {
        while (getline(&line, &n, fp) != -1)
            wprintw(pad, "%s", line);

        fclose(fp);
        if (line != NULL)
            free(line);
    } else
        wprintw(pad, "can not open '%s'\n", filename);

    prefresh(pad, 0, 0, y, x,
             height + y - 1, width + x - 1);

    keypad(pad, TRUE);
    while(1) {
        getch_key = wgetch(pad);

        if (getch_key == KEY_UP) {
            if (pad_row > 0)
                pad_row--;
        } else if (getch_key == KEY_DOWN) {
            if (pad_row < ssize - height)
                pad_row++;
        } else if (getch_key == KEY_RESIZE) {
            ungetch(getch_key);
            break;
        } else
            break;

        prefresh(pad, pad_row, 0,
                 y, x, height + y - 1, width + x - 1);
    }

    delwin(pad);
    return OK;
}

int open_message_box(int height, int width,
                     int y, int x, const char *message, const char *keys[])
{
    int getch_key;
    WINDOW *wpopup, *wmessage;

    if ((wpopup = draw_popup(height,
                             width, y, x, keys)) == NULL)
        return ERR;

    if ((wmessage = derwin(wpopup,
                           height, width - 2, 1, 2)) == NULL) {
        delwin(wpopup);
        return ERR;
    }

    mvwprintw(wmessage, 0, 0, message);

    keypad(wpopup, TRUE);
    getch_key = wgetch(wpopup);

    delwin(wmessage);
    delwin(wpopup);

    return getch_key;
}

_string_t open_input_box(int height, int width,
                         int y, int x, const char *info,	const char *message,
                         _string_t initial, const char *regexp)
{
#define caption fields[0] /* ... caption field of input box. */
#define textbox fields[1] /* ... textbox field of input box. */

    FORM *form;
    FIELD *fields[3] = { NULL };
    WINDOW *wpopup, *wmessage, *wform = NULL;
    int getch_key;

    _string_t ret = NULL;

    static const char *keys[] =   /* ... some info for user. */
    {
        "Press [%bReturn%r] to confirm or [%bF5%r] to cancel.",
        NULL
    };

    if ((wpopup = draw_popup(height,
                             width, y, x, keys)) == NULL)
        return NULL;

    if ((wmessage = derwin(wpopup, height - 1, width - 2, 1, 2)) == NULL ||
        (wform = derwin(wpopup, 1, width - 2, height, 2)) == NULL)
        goto input_box_exit;

    mvwprintw(wmessage, 0, 0, info);

    /* ... allocate fields. */
    int caps_sz = strlen(message);
    if ((caption = new_field(1, caps_sz, 0, 0, 0, 0)) == NULL)
        goto input_box_exit;

    if ((textbox = new_field(1, width - caps_sz - 3,
                             0, caps_sz + 1, 0, 0)) == NULL) {
        free_field(caption);
        goto input_box_exit;
    }

    set_field_buffer(caption, 0, message);
    set_field_opts(caption, O_STATIC | O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
    set_field_back(caption, COLOR_PAIR(1));

    set_field_buffer(textbox, 0, initial);
    set_field_opts(textbox, O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
    set_field_back(textbox, A_BOLD);
    set_field_type(textbox, TYPE_REGEXP, regexp);

    if ((form = new_form(fields)) == NULL) {
        free_field(caption);
        free_field(textbox);
        goto input_box_exit;
    }

    set_form_win(form, wpopup);
    set_form_sub(form, wform);
    post_form(form);

    curs_set(1);
    keypad(wpopup, TRUE);
    while(1) {
        getch_key = wgetch(wpopup);
        if (getch_key == KEY_RESIZE)
            break;

        if (getch_key == KEY_ENTER ||
            getch_key == '\n') {
            if (form_driver(form, REQ_VALIDATION) != E_OK)
                continue;

            _string_t tmp = field_buffer(textbox, 0);

            int n = strlen(tmp);
            /* ... trimed and null-terminated string. */
            while(n > 2 && isspace(tmp[n - 1])) n--;
            ret = strndup(tmp, n);
            break;
        }

        /* ... user canceled us. */
        if (getch_key == KEY_F(5))
            break;

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

    unpost_form(form);
    free_form(form);
    free_field(caption);
    free_field(textbox);
    curs_set(0);

input_box_exit:
    delwin(wmessage);
    delwin(wform);
    delwin(wpopup);
    return ret;
}

int open_radio_box(int height, int width, int y, int x,
                   const char *message, _string_t choices[],
                   int max_row, int selected, int ssize)
{

    MENU *menu;
    ITEM **items;
    WINDOW *wpopup, *wmessage, *winput = NULL;
    int getch_key, i;

    int ret = -1;

    static const char *keys[] = {
        "Press [%bReturn%r] to confirm or [%bF5%r] to cancel.",
        NULL
    };

    if ((wpopup = draw_popup(height,
                             width, y, x, keys)) == NULL)
        return -1;

    if ((wmessage = derwin(wpopup, height - ssize,
                           width - 2, 1, 2)) == NULL ||
        (winput = derwin(wpopup, ssize,	width - 2,
                         height - ssize + 1, 2)) == NULL)
        goto radio_box_exit;

    mvwprintw(wmessage, 0, 0, message);

    items = alloca((max_row + 1) * sizeof(ITEM *));
    for(i = 0; i < max_row; i++) {
        items[i] = new_item(choices[i],
                            (i == selected) ? "[*]" : "[ ]");
        if (items[i] == NULL)
            goto radio_box_exit_item;
    }

    items[i] = NULL;

    if ((menu = new_menu(items)) == NULL)
        goto radio_box_exit_item;

    set_menu_win(menu, wpopup);
    set_menu_sub(menu, winput);
    set_menu_mark(menu, "  ");
    set_menu_format(menu, ssize, 1);
    set_menu_back(menu, COLOR_PAIR(1));
    post_menu(menu);

    keypad(wpopup, TRUE);
    while(1) {
        getch_key = wgetch(wpopup);
        if (getch_key == KEY_RESIZE)
            break;

        if (getch_key == KEY_ENTER ||
            getch_key == '\n') {
            ret = item_index(current_item(menu));
            break;
        }

        /* ... user canceled us. */
        if (getch_key == KEY_F(5))
            break;

        switch(getch_key) {
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

radio_box_exit_item:
    while(i > 0)
        free_item(items[--i]);

radio_box_exit:
    delwin(wmessage);
    delwin(winput);
    delwin(wpopup);
    return ret;
}
