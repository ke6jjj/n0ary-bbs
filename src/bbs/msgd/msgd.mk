# Get the directory in which this Makefile fragment resides.
MSGD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
MSGD_APP_OBJDIR := $(BBS_OBJDIR)/msgd

MSGD_APP_PROD := b_msgd

MSGD_APP_SRCS := \
	main.c read.c rfc822.c list.c parse.c edit.c fwd.c bid.c

$(eval $(call create_bbs_app_rules,MSGD_APP))

BBS_PRODUCTS += $(MSGD_APP)
BBS_CLEAN_TARGETS += MSGD_APP_CLEAN
BBS_INSTALL_TARGETS += MSGD_APP_INSTALL
