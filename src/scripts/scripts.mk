#
# Get the directory in which this Makefile fragment resides.
#
SCRIPTS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

STARTUP_SCRIPTS_SRCS := n0ary_bbs.sh
AUTOMATED_SCRIPTS_SRCS := callbook-update-uls.sh bbs-inetd.sh bbs-init.sh

BSD_STARTUP_DIR ?= /usr/local/etc/rc.d
BSD_AUTOMATED_DIR ?= /usr/local/libexec

SCRIPTS_INSTALL:
	-@# Ensure old startup file is removed.
	-@rm $(BSD_STARTUP_DIR)/n0ary-bbs
	install -d $(BSD_STARTUP_DIR)
	for sc in $(STARTUP_SCRIPTS_SRCS); do \
		shless=$$( basename $$sc .sh ); \
		sed -e s,XXBBS_DIRXX,$(BBS_DIR), < $(SCRIPTS_SRCDIR)/$$sc \
		> $(BSD_STARTUP_DIR)/$$shless; \
		chmod 755 $(BSD_STARTUP_DIR)/$$shless; \
	done
	install -d $(BSD_AUTOMATED_DIR)
	for sc in $(AUTOMATED_SCRIPTS_SRCS); do \
		shless=$$( basename $$sc .sh ); \
		sed -e s,XXBBS_DIRXX,$(BBS_DIR), < $(SCRIPTS_SRCDIR)/$$sc \
		> $(BSD_AUTOMATED_DIR)/$$shless; \
		chmod 755 $(BSD_AUTOMATED_DIR)/$$shless; \
	done
