/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <ncurses.h>

#include "ncurses.gui.h"
#include "y.tab.h"

static const char screen_title[] = { SCREEN_TITLE };

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
    union {
        menu_t *menu;
        item_t *item;
#define eo_config(c) ((c)->__ptr_entry == NULL)
        void *__ptr_entry;
    };
} config_t;

static WINDOW *main_screen;
#define screen_subwin(...) subwin(main_screen, __VA_ARGS__)
static WINDOW *middle = NULL, *footnote = NULL;

static int init_screen(void)
{
    static int X = 0, Y = 0;

    if ((X != COLS) || (Y != LINES)) {
        clear();
        delwin(middle);
        delwin(footnote);

        if (((middle = screen_subwin(__MAIN_MENU_HIGH,
                        SCREEN_WIDTH,
                        TITLE_HIGH,
                        MARGIN_LEFT)) == NULL) ||
            ((footnote = screen_subwin(FOOTNOTE_HIGH,
                        SCREEN_WIDTH,
                        LINES - FOOTNOTE_HIGH - 1, MARGIN_LEFT)) == NULL)) {
            delwin(middle);     /* ... ignore NULL. */
            delwin(footnote);
            return -1;
        }

        getmaxyx(main_screen, Y, X);
    }

    return SUCCESS;
}

static int draw_screen(void)
{
    int i;

    if (init_screen() != SUCCESS)
        return -1;

    wborder(main_screen, 32, 32, 32, 32, 32, 32, 32, 32);
    wattrpr(main_screen, TITLE_LINE,
        (COLS / 2 - strlen(screen_title) / 2), screen_title);

    for (i = 0; (i < FOOTNOTE_HIGH) || (footnote_message[i] != NULL); i++)
        wattrpr(footnote, i, 0, footnote_message[i]);

    return SUCCESS;
}

#define PROMPT_MENU(_e) (_e).menu->prompt
#define PROMPT_ITEM(_e) (_e).item->common.prompt
static void draw_main_menu(const char *menu_title,
    config_t choices[], int current_highlight, int choice_start)
{
    int menu_high = 0;
    int current_row = choice_start;

    draw_screen();
    werase(middle);
    wattron(middle, A_BOLD);
    mvwprintw(middle, 0, 0, "[-] %s", menu_title);
    wattroff(middle, A_BOLD);

    while (!eo_config(&choices[current_row]) && (menu_high < MAIN_MENU_HIGH)) {

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

        default:
            mvwprintw(middle, menu_high + 2, 0,
                "err. unrecognised configuration item.");
        }

        if (current_row == current_highlight)
            wattroff(middle, A_STANDOUT);

        current_row++;
        menu_high++;
    }
}

#define SPECIAL_KEY_BS  -1      /* 'Backspace' pressed. */
#define SPECIAL_KEY_Q   -2      /* 'Q' pressed. */
#define SPECIAL_KEY_H   -3      /* 'H' pressed. */
#define SPECIAL_KEY_RT  -4      /* 'Retuen' pressed. */
#define SPECIAL_KEY_F1  -5      /* 'F1' pressed. */

static int selected_row = 0;
static int main_menu_driver(const char *menu_title, config_t choices[])
{
    int choice_start = 0;
    int max_row = 0, getch_key = 0;
    int pressed_key = SPECIAL_KEY_RT;

    while (!eo_config(&choices[max_row]))
        max_row++;

    if (selected_row >= max_row)
        selected_row = 0;

    keypad(main_screen, TRUE);

    while ((getch_key != KEY_ENTER) && (getch_key != '\n')) {
        if ((COLS > TERMINAL_COLS) && (LINES > TERMINAL_LINES)) {

            if (getch_key == KEY_UP) {
                if (selected_row > 0)
                    selected_row--;

                if (selected_row < choice_start && choice_start > 0)
                    choice_start--;
            }

            if (getch_key == KEY_DOWN) {
                if (selected_row < max_row - 1) {
                    selected_row++;

                    if (selected_row - choice_start >= MAIN_MENU_HIGH)
                        choice_start++;
                }
            }

            draw_main_menu(menu_title, choices, selected_row, choice_start);

            /* Processing special keys ... */

            if (getch_key == KEY_BACKSPACE) {
                pressed_key = SPECIAL_KEY_BS;
                break;
            }

            if (getch_key == 'q') {
                pressed_key = SPECIAL_KEY_Q;
                break;
            }

            if (getch_key == 'h') {
                pressed_key = SPECIAL_KEY_H;
                break;
            }

            if (getch_key == KEY_F(1)) {
                pressed_key = SPECIAL_KEY_F1;
                break;
            }

            /* Refresh 'main_screen', here. */
            getch_key = getch();

        } else {
            clear();
            mvwprintw(main_screen, 0, 0, "%s", "Terminal is too small ...");

            getch();            /* ... ignore the input. */
        }
    }

    keypad(main_screen, FALSE);

    return pressed_key;
}

#define array_realloc(arr, n) ({ \
        typeof(arr) tmp = realloc((arr), (n) * sizeof(arr[0])); \
        if (tmp == NULL) \
            free((arr)); \
        (tmp); \
    })

static config_t *get_config(menu_t * parent)
{
    static int conf_size = 0;
    static config_t *conf = NULL;

    menu_t *menu;
    item_t *item;
    int num = 1;

    LIST_FOREACH(menu, &parent->childs, sibling) {
        if (eval_expr(menu->dependency)) {

            if (++num > conf_size) {
                if ((conf = array_realloc(conf, num)) == NULL)
                    return NULL;
            }

            conf[num - 2].menu = menu;
            conf[num - 2].t = CONF_MENU;
        }
    }

    LIST_FOREACH(item, &parent->entries, node) {
        if ((item->common.prompt != NULL) && eval_expr(item->common.dependency)) {
            struct extended_token *et;

            if (++num > conf_size) {
                if ((conf = array_realloc(conf, num)) == NULL)
                    return NULL;
            }

            conf[num - 2].item = item;

            if ((et = item_get_config_et(item)) != NULL) {
                if (et->token.ttype == TT_BOOL)
                    conf[num - 2].t = et->token.TK_BOOL ? CONF_YES : CONF_NO;
                else
                    conf[num - 2].t = CONF_INPUT;
            } else
                conf[num - 2].t = CONF_RADIO;
        }
    }

    /* 'eo_config' set end-of-config. */
    conf[num - 1].__ptr_entry = NULL;

    if (num > conf_size)
        conf_size = num;

    return conf;
}

static int open_radio_item(item_t * item)
{
    string_t *choices = NULL;
    struct extended_token *et;
    int num = 0, selected = -1;

    item_token_list_for_each_entry(et, item) {
        if ((choices = array_realloc(choices, ++num)) == NULL)
            return -1;

        if (et->token.ttype == TT_INTEGER) {
            /* ... allocate from stack to simplify the cleanup. */
            choices[num - 1] = alloca(64);

            snprintf(choices[num - 1], 64,
                et->token.info.number.base == 16 ? "0x%X" : "%d",
                et->token.TK_INTEGER);

        } else                  /* and TT_DESCRIPTION. */
            choices[num - 1] = et->token.TK_STRING;

        if (et->flags & TK_LIST_EF_SELECTED)
            selected = num - 1;
    }

    selected = radio_box("", choices, num, selected, (num > 5) ? 5 : num);

    if (selected != -1)
        toggle_choice(item, choices[selected]);

    free(choices);

    return SUCCESS;
}

int start_gui(int nr_pages)
{
#define cur_config config[selected_row]
    config_t *config;

    int pressed_key, ret = SUCCESS, index = 0;

    main_screen = initscr();

    curs_set(0);
    start_color();
    cbreak();
    noecho();
    set_escdelay(0);
    init_pair(1, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);

    menu_t **stack = calloc(nr_pages, sizeof(menu_t *));
    stack[index] = &main_menu;

    while (TRUE) {

        /* ... get the 'config' array for GUI from 'stack' top. */
        if ((config = get_config(stack[index])) == NULL) {
            ret = -2;
            break;
        }

        pressed_key = main_menu_driver((stack[index]->prompt == NULL) ?
            /* Menu title: 'Options' as main title. */
            "Options" : stack[index]->prompt, config);

        switch (pressed_key) {
        case SPECIAL_KEY_BS:
            if (index > 0)
                index--;

            break;

        case SPECIAL_KEY_Q:
            while (TRUE) {

                int d = open_message_box(2, 90, LINES / 2, COLS / 2 - 45,
                    "%bAre you sure?%r", get_keys(EXIT));

                if (d == 'x') {
                    ret = -1;
                    goto out;
                }

                else if ((d == 'S') || (d == 's')) {
                    goto out;   /* ... return SUCCESS. */

                } else if ((d == 'C') || (d == 'c') || (d == 27))
                    break;

                else if (d == KEY_RESIZE)
                    break;      /* ... terminal resized. */
            }

            break;

        case SPECIAL_KEY_H:
            if (cur_config.t != CONF_MENU) {
                item_t *item = cur_config.item;

                open_pad(__MAIN_MENU_HIGH, SCREEN_WIDTH,
                    TITLE_HIGH, MARGIN_LEFT, (item->common.help == NULL) ?
                    "No help provided." : item->common.help, 150);
            }

            break;

        case SPECIAL_KEY_F1:
            open_file(__MAIN_MENU_HIGH, SCREEN_WIDTH,
                /* ... 'README.md' is inside the '_IN_FOLDER' folder. */
                TITLE_HIGH, MARGIN_LEFT, "README.md");
            break;

        default:               /* Process 'SPECIAL_KEY_RT' ... */

            if (cur_config.t == CONF_MENU)
                stack[++index] = cur_config.menu;

            else if (cur_config.t == CONF_YES)
                toggle_config(cur_config.item);

            else if (cur_config.t == CONF_NO)
                toggle_config(cur_config.item);

            else if (cur_config.t == CONF_INPUT) {
                string_t input;
                struct extended_token *et = item_get_config_et(cur_config.item);

                if (et->token.ttype == TT_INTEGER) {
                    input =
                        int_input_box("", cur_config.item->common.prompt,
                        et->token.info.number);

                    if (input != NULL)
                        toggle_config(cur_config.item, strtol(input, NULL, 0));

                } else {        /* and TT_DESCRIPTION. */
                    input = input_box("",
                        cur_config.item->common.prompt, et->token.TK_STRING,
                        "");

                    if (input != NULL)
                        toggle_config(cur_config.item, input);
                }

                free(input);
            } else              /* and 'CONF_RADIO'. */
                open_radio_item(cur_config.item);
        }
    }

 out:
    free(config);
    free(stack);

    clear();
    nocbreak();
    echo();
    endwin();

    return ret;
}
