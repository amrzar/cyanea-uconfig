#  SPDX-License-Identifier: GPL-2.0-or-later

configs.in ?= $(srctree)/configs.in

DEPS = $(wildcard *.d)
SOURCES = db.c main.c ncurses.gui.c gui.c

-include $(DEPS)

y.tab.c: config.parser.y
	@echo "YY      $@"
	$(Q)yacc -d $< --debug --verbose # Genetate y.tab.h as well.

lex.yy.c: config.parser.l
	@echo "LX      $@"
	$(Q)lex $<

config.ncurses: y.tab.o lex.yy.o $(patsubst %.c,%.o,$(SOURCES))
	@echo "LD      $@"
	$(Q)$(HOSTCC) $^ -lncurses -lmenu -lform -ly -o $@

%.o: %.c
	@echo "CC      $<"
	$(Q)$(HOSTCC) $(HOSTCFLAGS) -MMD -MF $(patsubst %.o,%.d,$@) -c -o $@ $<

menuconfig: config.ncurses FORCE
	$(Q)./config.ncurses --config $(configs.in) --sys-config $(sysconfig) --gui

silentoldconfig: config.ncurses FORCE
	$(Q)./config.ncurses --config $(configs.in) --sys-config $(sysconfig)

defconfig: config.ncurses FORCE
	$(Q)rm -f $(dir $(configs.in)).old.config
	$(Q)./config.ncurses --dump --config $(configs.in)

style:
	$(Q)find . \( -name '*.c' -o -name '*.h' \) -exec ../scripts/style.sh {} ';' 

clean:
	$(Q)rm -f lex.yy.c y.tab.c y.output y.tab.h \
		$(wildcard *.o) config.ncurses $(DEPS)

FORCE:
.PHONY: style clean FORCE
