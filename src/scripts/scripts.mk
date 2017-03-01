#
# Get the directory in which this Makefile fragment resides.
#
SCRIPTS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

SCRIPTS_SRCS := n0ary-bbs.sh

BSD_STARTUP_DIR ?= /usr/local/etc/rc.d

SCRIPTS_INSTALL:
	install -d $(BSD_STARTUP_DIR)
	for sc in $(SCRIPTS_SRCS); do \
		sed -e s,XXBBS_DIRXX,$(BBS_DIR), < $(SCRIPTS_SRCDIR)/$$sc \
		> $(BSD_STARTUP_DIR)/$$sc; \
		chmod 755 $(BSD_STARTUP_DIR)/$$sc; \
	done
