# Get the directory in which this Makefile fragment resides.
DECTALK_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
DECTALK_APP_OBJDIR := $(BBS_OBJDIR)/dectalk

DECTALK_APP_PROD := b_dectalk

DECTALK_APP_SRCS := dectalk.c

$(eval $(call create_bbs_app_rules,DECTALK_APP))

ifneq ($(ENABLE_DECTALK),0)
  BBS_PRODUCTS += $(DECTALK_APP)
  BBS_CLEAN_TARGETS += DECTALK_APP_CLEAN
  BBS_INSTALL_TARGETS += DECTALK_APP_INSTALL
endif
