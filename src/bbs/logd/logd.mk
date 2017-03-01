# Get the directory in which this Makefile fragment resides.
LOGD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
LOGD_APP_OBJDIR := $(BBS_OBJDIR)/logd

LOGD_APP_PROD := b_logd

LOGD_APP_SRCS := \
	main.c

$(eval $(call create_bbs_app_rules,LOGD_APP))

BBS_PRODUCTS += $(LOGD_APP)
BBS_CLEAN_TARGETS += LOGD_APP_CLEAN
BBS_INSTALL_TARGETS += LOGD_APP_INSTALL
