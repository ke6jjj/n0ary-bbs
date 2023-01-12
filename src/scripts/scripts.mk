#
# Get the directory in which this Makefile fragment resides.
#
SCRIPTS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

STARTUP_SCRIPTS_SRCS := n0ary-bbs.sh
AUTOMATED_SCRIPTS_SRCS := callbook-update-uls.sh bbs-inetd.sh bbs-init.sh

BSD_STARTUP_DIR ?= /usr/local/etc/rc.d
BSD_AUTOMATED_DIR ?= /usr/local/libexec

SCRIPTS_INSTALL:
	install -d $(BSD_STARTUP_DIR)
	for sc in $(STARTUP_SCRIPTS_SRCS); do \
		sed -e s,XXBBS_DIRXX,$(BBS_DIR), < $(SCRIPTS_SRCDIR)/$$sc \
		> $(BSD_STARTUP_DIR)/$$sc; \
		chmod 755 $(BSD_STARTUP_DIR)/$$sc; \
	done
	install -d $(BSD_AUTOMATED_DIR)
	for sc in $(AUTOMATED_SCRIPTS_SRCS); do \
		sed -e s,XXBBS_DIRXX,$(BBS_DIR), < $(SCRIPTS_SRCDIR)/$$sc \
		> $(BSD_AUTOMATED_DIR)/$$sc; \
		chmod 755 $(BSD_AUTOMATED_DIR)/$$sc; \
	done
