/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__

#define SCREEN_TITLE ".:%bCyanea ukernel%r Configuration:."

#define TERMINAL_LINES 30
#define TERMINAL_COLS 100

#define TITLE_LINE 2            /* Index of screen's title line. */
#define TITLE_HIGH 6            /* Number of lines in title section. */
#define FOOTNOTE_HIGH 2         /* Number of lines in footnote section. */
#define MARGIN_LEFT 2           /* Index of screen's margin. */

#define _IN_FILE "configs.in"
#define _OUT_FILE "sys.config.h"

#ifdef DEBUG
#define debug_print(...) \
    fprintf(stderr, "[debug_print] " __VA_ARGS__)
#else
#define debug_print(...)
#endif

#define error_print(...) do { \
        fprintf(stderr, "[error_print] %s ", __FUNCTION__); \
        fprintf(stderr, __VA_ARGS__); \
    } while (0)

#endif /* __DEFAULTS_H__ */
