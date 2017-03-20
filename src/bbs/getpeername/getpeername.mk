# Get the directory in which this Makefile fragment resides.
GETPEERNAME_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
GETPEERNAME_APP_OBJDIR := $(BBS_OBJDIR)/getpeername

GETPEERNAME_APP_PROD := getpeername

GETPEERNAME_APP_SRCS := main.c

$(eval $(call create_bbs_app_rules,GETPEERNAME_APP))

BBS_PRODUCTS += $(GETPEERNAME_APP)
BBS_CLEAN_TARGETS += GETPEERNAME_APP_CLEAN
BBS_INSTALL_TARGETS += GETPEERNAME_APP_INSTALL
