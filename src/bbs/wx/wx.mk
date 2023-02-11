# Get the directory in which this Makefile fragment resides.
WX_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
WX_OBJDIR := $(BBS_OBJDIR)/wx

WX_APP_SRCS := wx.py wx-weewx-local.py

$(eval $(call create_bbs_app_rules,CALLBK_MAKECB_ULS_APP))

BBS_PRODUCTS += $(CALLBK_MAKECB_ULS_APP)
BBS_CLEAN_TARGETS += CALLBK_MAKECB_ULS_APP_CLEAN
BBS_INSTALL_TARGETS += CALLBK_MAKECB_ULS_APP_INSTALL
