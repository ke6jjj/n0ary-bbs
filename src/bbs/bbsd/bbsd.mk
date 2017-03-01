# Get the directory in which this Makefile fragment resides.
BBSD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BBSD_APP_OBJDIR := $(BBS_OBJDIR)/bbsd

BBSD_APP_PROD := b_bbsd

BBSD_APP_SRCS := main.c parse.c config.c lock.c daemon.c

$(eval $(call create_bbs_app_rules,BBSD_APP))

BBS_PRODUCTS += $(BBSD_APP)
BBS_CLEAN_TARGETS += BBSD_APP_CLEAN
BBS_INSTALL_TARGETS += BBSD_APP_INSTALL
