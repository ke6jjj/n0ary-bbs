# Get the directory in which this Makefile fragment resides.
CALCLOAD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CALCLOAD_APP_OBJDIR := $(BBS_OBJDIR)/calcload

CALCLOAD_APP_PROD := b_calcload

CALCLOAD_APP_SRCS := calcload.c

CALCLOAD_APP_CFLAGS := -DHISTFILE=\"/dev/null\"

$(eval $(call create_bbs_app_rules,CALCLOAD_APP))

ifneq ($(ENABLE_CALCLOAD),0)
  BBS_PRODUCTS += $(CALDLOAD_APP)
  BBS_CLEAN_TARGETS += CALCLOAD_APP_CLEAN
endif
