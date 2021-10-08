/*
 * Copyright 2021 Amirreza Zarrabi <amrzar@gmail.com>
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

#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__

#define SCREEN_TITLE ".:%bCyanea ukernel%r Configuration:."
#define FOOTNOOTE_MESSAGE                                               \
    "Press [%bReturn%r] to select and [%bBackspace%r] to go back ...",  \
    "Press [%bh%r] for help and [%bq%r] to exit."

#define TERMINAL_LINES 30
#define TERMINAL_COLS 100

#define TITLE_LINE 2    /* ... index of screen's title line. */
#define TITLE_HIGH 6    /* ... number of lines in title section. */
#define FOOTNOTE_HIGH 2 /* ... number of lines in footnote section. */
#define MARGIN_LEFT 2   /* ... index of screen's margin. */

#define _IN_FILE "configs.in"
#define _OUT_FILE "sys.config.h"

#endif /* __DEFAULTS_H__ */