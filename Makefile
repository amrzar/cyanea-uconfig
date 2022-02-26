#  SPDX-License-Identifier: GPL-2.0-or-later

DEPS = $(wildcard *.d)
SOURCES = config.db.c main.c ncurses.gui.c gui.c

-include $(DEPS)

y.tab.c: config.parser.y
	@echo "YY $@"
	$(Q)yacc -d $< --debug --verbose # Genetate y.tab.h as well.

lex.yy.c: config.parser.l
	@echo "LX $@"
	$(Q)lex $<

config.ncurses: y.tab.o lex.yy.o $(patsubst %.c,%.o,$(SOURCES))
	@echo "LD $@"
	$(Q)$(HOSTCC) $^ -lncurses -lmenu -lform -ly -o $@

%.o: %.c
	@echo "CC $<"
	$(Q)$(HOSTCC) $(HOSTCFLAGS) -MMD -MF $(patsubst %.o,%.d,$@) -c -o $@ $<

menuconfig: config.ncurses
	$(Q)./config.ncurses -i $(CONFIG) -o $(SYS_CONFIG) -u

silentoldconfig: config.ncurses
	$(Q)./config.ncurses -i $(CONFIG) -o $(SYS_CONFIG)

defconfig: config.ncurses
	$(Q)rm -f $(dir $(CONFIG)).old.config
	$(Q)./config.ncurses -C -i $(CONFIG)

style:
	$(Q)astyle \
	--style=attach \
	--indent-after-parens \
	--indent-preproc-define \
	--convert-tabs \
	--indent-labels \
	--indent-preproc-cond \
	--break-blocks \
	--pad-oper \
	--pad-comma \
	--pad-header \
	--align-pointer=name \
	--break-one-line-headers \
	$(SOURCES) \
		config.db.h config.parser.h config.utils.h ncurses.gui.h defaults.h

clean:
	$(Q)rm -f lex.yy.c y.tab.c y.output y.tab.h \
		$(wildcard *.o) config.ncurses $(DEPS)

.PHONY: menuconfig silentoldconfig defconfig style clean
