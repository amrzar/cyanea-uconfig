/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__

#define SCREEN_TITLE ".:%bCyanea ukernel%r Configuration:."
#define FOOTNOOTE_MESSAGE                                               \
    "Press [%bReturn%r] to select and [%bBackspace%r] to go back ...",  \
    "Press [%bh%r] for help and [%bq%r] to exit."

#define TERMINAL_LINES 30
#define TERMINAL_COLS 100

#define TITLE_LINE 2            /* ... index of screen's title line. */
#define TITLE_HIGH 6            /* ... number of lines in title section. */
#define FOOTNOTE_HIGH 2         /* ... number of lines in footnote section. */
#define MARGIN_LEFT 2           /* ... index of screen's margin. */

#define _IN_FILE "configs.in"
#define _OUT_FILE "sys.config.h"

#endif /* __DEFAULTS_H__ */
