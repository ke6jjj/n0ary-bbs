# Get the directory in which this Makefile fragment resides.
USERD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
USERD_APP_OBJDIR := $(BBS_OBJDIR)/userd

USERD_APP_PROD := b_userd

USERD_APP_SRCS := \
	main.c usrdir.c parse.c usrfile.c edit.c age.c disp.c

$(eval $(call create_bbs_app_rules,USERD_APP))

BBS_PRODUCTS += $(USERD_APP)
BBS_CLEAN_TARGETS += USERD_APP_CLEAN
