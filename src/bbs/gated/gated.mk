# Get the directory in which this Makefile fragment resides.
GATED_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
GATED_APP_OBJDIR := $(BBS_OBJDIR)/gated

GATED_APP_PROD := b_gated

GATED_APP_SRCS := \
	main.c file.c date.c parse.c mail.c guess.c age.c

$(eval $(call create_bbs_app_rules,GATED_APP))

BBS_PRODUCTS += $(GATED_APP)
BBS_CLEAN_TARGETS += GATED_APP_CLEAN
