
 #
 # Copyright 2020 Amirreza Zarrabi <amrzar@gmail.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation; either version 2 of the License, or
 # (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #

Q := @
CC := gcc

DEPS := $(wildcard *.d)

all: config.ncurses

-include $(DEPS)

y.tab.c: config.parser.y
	$(Q)echo "YY $@"
	$(Q)yacc -d $< --debug --verbose # Genetate y.tab.h as well.

lex.yy.c: config.parser.l
	$(Q)echo "LX $@"
	$(Q)lex $<

config.ncurses: y.tab.o lex.yy.o config.db.o ncurses.utils.o ncurses.gui.o
	$(Q)echo "LD $@"
	$(Q)$(CC) $^ -lncurses -lmenu -lform -o $@

%.o: %.c
	$(Q)echo "CC $<"
	$(Q)$(CC) $(CFLAGS) -MMD -MF $(patsubst %.o,%.d,$@) -c -o $@ $<

menuconfig: config.ncurses
	$(Q)./config.ncurses

oldconfig: config.ncurses
	$(Q)rm .old.config > /dev/null 2>&1 || true
	$(Q)./config.ncurses -C

clean:
	$(Q)echo "[CLEAN]"
	$(Q)rm lex.yy.c y.tab.c y.output y.tab.h $(wildcard *.o) \
	config.ncurses $(DEPS) > /dev/null 2>&1 || true

.PHONY: all clean