# Get the directory in which this Makefile fragment resides.
NEWTNCD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
NEWTNCD_APP_OBJDIR := $(BBS_OBJDIR)/newtncd

NEWTNCD_APP_PROD := b_newtncd

NEWTNCD_APP_SRCS :=

$(eval $(call create_bbs_app_rules,NEWTNCD_APP))

ifdef BUILD_NEWTNCD
BBS_PRODUCTS += $(NEWTNCD_APP)
BBS_CLEAN_TARGETS += NEWTNCD_APP_CLEAN
BBS_INSTALL_TARGETS += NEWTNCD_APP_INSTALL
endif
