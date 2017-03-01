# Get the directory in which this Makefile fragment resides.
BBSTOOL_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BBSTOOL_APP_OBJDIR := $(BBS_OBJDIR)/bbstool

BBSTOOL_APP_PROD := b_tool

BBSTOOL_APP_SRCS := \
	tool.c usrwin.c lckwin.c dmnwin.c bbsd.c monitor.c msgwin.c

BBSTOOL_APP_CFLAGS := -I/usr/X11R6/include
BBSTOOL_APP_LDFLAGS := -lXm -lXt -lX11 -L/usr/X11R6/lib

$(eval $(call create_bbs_app_rules,BBSTOOL_APP))

ifneq ($(ENABLE_BBSTOOL),0)
BBS_PRODUCTS += $(BBSTOOL_APP)
BBS_CLEAN_TARGETS += BBSTOOL_APP_CLEAN
BBS_INSTALL_TARGETS += BBSTOOL_APP_INSTALL
endif
