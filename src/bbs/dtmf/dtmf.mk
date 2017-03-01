# Get the directory in which this Makefile fragment resides.
DTMF_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
DTMF_APP_OBJDIR := $(BBS_OBJDIR)/dtmf

DTMF_APP_PROD := dtmf

DTMF_APP_SRCS := dtmf.c

$(eval $(call create_bbs_app_rules,DTMF_APP))

ifneq ($(ENABLE_DECTALK),0)
  BBS_PRODUCTS += $(DTMF_APP)
  BBS_CLEAN_TARGETS += DTMF_APP_CLEAN
  BBS_INSTALL_TARGETS += DTMF_APP_INSTALL
endif
