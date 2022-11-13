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

#endif /* __DEFAULTS_H__ */
