/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <ctype.h>
#include <ncurses.h>
#include <form.h>
#include <menu.h>

#include "ncurses.gui.h"

static _key_t okcancel_keys(void)
{
    static const char *keys[] = {
        "[%bReturn%r]" "[%bEsc%r]",
        NULL
    };

    return keys;
}

static inline string_t trim_tail(string_t str)
{
    int slen = strlen(str);

    while (isspace(str[slen - 1]) != 0)
        slen--;

    return strndup(str, slen);
}

int wattrpr(WINDOW * win, int y, int x, const char *s)
{
    int attr = 0, slen = 0;
    wmove(win, y, x);

    for (int i = 0; i < strlen(s); i++) {
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
            }
        } else {
            slen++;
            waddch(win, s[i] | attr);
        }
    }

    return slen;
}

int GUI_FUNCTION(open_newpad, const char *message, int area)
{
    WINDOW *pad;

    if ((message == NULL) ||
        (height > area) || ((pad = newpad(area, width)) == NULL))
        return ERR;

    wprintw(pad, "%s", message);
    keypad(pad, TRUE);

    while (TRUE) {
        int getch_key, pad_row = 0;

        prefresh(pad, pad_row, 0, y, x, height + y - 1, width + x - 1);

        if ((getch_key = wgetch(pad)) == KEY_UP) {
            if (pad_row > 0)
                pad_row--;
        } else if (getch_key == KEY_DOWN) {
            if (pad_row < area - height)
                pad_row++;
        } else
            break;
    }

    delwin(pad);
    return OK;
}

#define POPUP __popup
#define popup_getchar() wgetch(POPUP)
static WINDOW *__popup, *__message;

static int GUI_FUNCTION(open_popup, int H, const char *message, _key_t keys)
{
    int offset, i;

    POPUP = newwin(height + 4, width + 2, y - 1, x - 1);

    if (POPUP == NULL)
        return ERR;

    wbkgd(POPUP, POPUP_COLOR_ATTR);
    wborder(POPUP, 0, 0, 0, 0, 0, 0, 0, 0);

    /* Here, 'H' is the height of blank space bellow the '__message'
     * windows. It is used for input boxes such as textbox.  */

    __message = derwin(POPUP, height - H, width - 2, 1, 2);

    if (__message == NULL) {
        delwin(POPUP);
        return ERR;
    }

    wbkgd(__message, POPUP_COLOR_ATTR);
    wattrpr(__message, 0, 0, message);

    for (i = 0, offset = 2; keys[i] != NULL; i++, offset++)
        offset += wattrpr(POPUP, height + 2, offset, keys[i]);

    return OK;
}

static int close_popup(void)
{
    if (delwin(POPUP) == ERR || delwin(__message) == ERR)
        return ERR;

    return OK;
}

int GUI_FUNCTION(open_textfile, const char *path)
{
    FILE *fp;
    string_t tmp;

    if ((fp = fopen(path, "r")) == NULL)
        return -1;

    fseek(fp, 0, SEEK_END);
    size_t n = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if ((tmp = malloc(n + 1)) == NULL) {
        fclose(fp);
        return -1;
    }

    tmp[fread(tmp, 1, n, fp)] = '\0';
    fclose(fp);

    int ret = 0;

    /* ... assume '10 * height' as scrollable area. */
    if (open_newpad(height, width, y, x, tmp, 10 * height) == ERR)
        ret = -1;

    free(tmp);
    return ret;
}

int GUI_FUNCTION(open_message_box, const char *message, _key_t keys)
{
    int getch_key;

    if (open_popup(height, width, y, x, 0, message, keys) == ERR)
        return -1;

    keypad(POPUP, TRUE);
    getch_key = popup_getchar();
    close_popup();

    return getch_key;
}

string_t GUI_FUNCTION(open_input_box,
    const char *help,
    const char *message, const char *initial, const char *regexp)
{

    WINDOW *form_win;
    FORM *form;
    FIELD *fields[3] = { NULL };
    string_t input = NULL;
    size_t n;

    if ((n = strlen(message)) > width + 2)
        return NULL;

    if (open_popup(height, width, y, x, 1, help, okcancel_keys()) == ERR)
        return NULL;

    if ((form_win = derwin(POPUP, 1, width - 2, height, 2)) == NULL)
        goto input_box_exit;

    /* ... assuming a width, [1][n][1][width - n - 3][1]. */
    if ((fields[0] = new_field(1, n, 0, 0, 0, 0)) == NULL)
        goto input_box_exit;

    if ((fields[1] = new_field(1, width - n - 3, 0, n + 1, 0, 0)) == NULL) {
        free_field(fields[0]);
        goto input_box_exit;
    }
#define init_field(_w, _m, _a, _o) do {         \
        set_field_buffer((_w), 0, (_m));        \
        set_field_opts((_w), (_o));             \
        set_field_back((_w), (_a));             \
    } while (0)

    init_field(fields[0], message, POPUP_COLOR_ATTR,
        O_STATIC | O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
    init_field(fields[1], initial, A_BOLD,
        O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);
#undef init_field

    set_field_type(fields[1], TYPE_REGEXP, regexp);

    if ((form = new_form(fields)) == NULL)
        goto input_box_exit_form_field;

    set_form_win(form, POPUP);
    set_form_sub(form, form_win);
    post_form(form);

    curs_set(1);
    keypad(POPUP, TRUE);

    while (TRUE) {
        int getch_key = popup_getchar();

        if (getch_key == KEY_RESIZE || getch_key == 27)
            break;

        if (getch_key == KEY_ENTER || getch_key == '\n') {
            if (form_driver(form, REQ_VALIDATION) != E_OK)
                continue;

            input = trim_tail(field_buffer(fields[1], 0));
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
    free_field(fields[0]);
    free_field(fields[1]);

 input_box_exit:
    delwin(form_win);
    close_popup();

    return input;
}

int GUI_FUNCTION(open_radio_box,
    const char *message,
    string_t choices[], int max_row, int selected, int max_radio_box_row)
{

    WINDOW *menu_win;
    MENU *menu;
    ITEM **items = alloca((max_row + 1) * sizeof(ITEM *));
    int n;

    if (open_popup(height, width, y, x,
            max_radio_box_row, message, okcancel_keys()) == ERR)
        return -1;

    menu_win = derwin(POPUP, max_radio_box_row, width - 2,
        height - max_radio_box_row + 1, 2);

    if (menu_win == NULL)
        goto radio_box_exit;

    for (n = 0; n < max_row; n++) {
        items[n] = new_item(choices[n], (n == selected) ? "[*]" : "[ ]");

        if (items[n] == NULL)
            goto radio_box_exit_from_item;
    }

    items[n] = NULL;

    if ((menu = new_menu(items)) == NULL)
        goto radio_box_exit_from_item;

    set_menu_win(menu, POPUP);
    set_menu_sub(menu, menu_win);
    set_menu_mark(menu, "  ");
    set_menu_format(menu, max_radio_box_row, 1);
    set_menu_back(menu, POPUP_COLOR_ATTR);
    post_menu(menu);

    keypad(POPUP, TRUE);

    while (TRUE) {
        int getch_key = popup_getchar();

        if (getch_key == KEY_RESIZE || getch_key == 27)
            break;

        if (getch_key == KEY_ENTER || getch_key == '\n') {
            selected = item_index(current_item(menu));
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

    while (n > 0)
        free_item(items[--n]);

 radio_box_exit:
    delwin(menu_win);
    close_popup();

    return selected;
}
