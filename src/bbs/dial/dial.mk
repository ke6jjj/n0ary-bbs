# Get the directory in which this Makefile fragment resides.
DIAL_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
DIAL_APP_OBJDIR := $(BBS_OBJDIR)/dial

DIAL_APP_PROD := b_dialin
DIAL_APP_SRCS := dialin.c

$(eval $(call create_bbs_app_rules,DIAL_APP))

DIALBS_APP_SRCDIR := $(DIAL_APP_SRCDIR)
DIALBS_APP_OBJDIR := $(BBS_OBJDIR)/dialBS
DIALBS_APP_PROD := b_dialinBS
DIALBS_APP_SRCS := $(DIAL_APP_SRCS)
DIALBS_APP_CFLAGS := -DBS

$(eval $(call create_bbs_app_rules,DIALBS_APP))

DIAL_APPS_CLEAN: DIAL_APP_CLEAN DIALBS_APP_CLEAN

BBS_PRODUCTS += $(DIAL_APP) $(DIALBS_APP)
BBS_CLEAN_TARGETS += DIAL_APPS_CLEAN
