# Get the directory in which this Makefile fragment resides.
BIDD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BIDD_APP_OBJDIR := $(BBS_OBJDIR)/bidd

BIDD_APP_PROD := b_bidd

BIDD_APP_SRCS := main.c read.c hash.c date.c parse.c write.c edit.c

$(eval $(call create_bbs_app_rules,BIDD_APP))

BBS_PRODUCTS += $(BIDD_APP)
BBS_CLEAN_TARGETS += BIDD_APP_CLEAN
