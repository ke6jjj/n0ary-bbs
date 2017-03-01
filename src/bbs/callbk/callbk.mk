# Get the directory in which this Makefile fragment resides.
CALLBK_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CALLBK_APP_OBJDIR := $(BBS_OBJDIR)/callbk

CALLBK_MAKECB_APP_SRCDIR := $(CALLBK_APP_SRCDIR)
CALLBK_MAKECB_APP_OBJDIR := $(CALLBK_APP_OBJDIR)
CALLBK_MAKECB_APP_PROD := make_cb
CALLBK_MAKECB_APP_SRCS := make_cb.c

$(eval $(call create_bbs_app_rules,CALLBK_MAKECB_APP))

CALLBK_SORTCB_APP_SRCDIR := $(CALLBK_APP_SRCDIR)
CALLBK_SORTCB_APP_OBJDIR := $(CALLBK_APP_OBJDIR)
CALLBK_SORTCB_APP_PROD := sort_cb
CALLBK_SORTCB_APP_SRCS := sort_cb.c

$(eval $(call create_bbs_app_rules,CALLBK_SORTCB_APP))

CALLBK_DISPCB_APP_SRCDIR := $(CALLBK_APP_SRCDIR)
CALLBK_DISPCB_APP_OBJDIR := $(CALLBK_APP_OBJDIR)
CALLBK_DISPCB_APP_PROD := disp_cb
CALLBK_DISPCB_APP_SRCS := disp_cb.c

$(eval $(call create_bbs_app_rules,CALLBK_DISPCB_APP))

CALLBK_APP_CLEAN:
	rm -rf $(CALLBK_APP_OBJDIR)

#
# This target has been preserved from the 1995 Makefile. I'm not sure
# what to do with it, yet.
#
CALLBK_CREATE: $(CALLBK_MAKECB_APP) $(CALLBK_SORTCB_APP)
	$(CALLBK_MAKECB_APP) . $(INS)/../callbk
	$(CALLBK_SORTCB_APP) $(INS)/../callbk/lastname.indx
	$(CALLBK_SORTCB_APP) $(INS)/../callbk/firstname.indx
	$(CALLBK_SORTCB_APP) $(INS)/../callbk/city.indx
	$(CALLBK_SORTCB_APP) $(INS)/../callbk/zip.indx

BBS_PRODUCTS += $(CALLBK_MAKECB_APP) $(CALLBK_DISPCB_APP) $(CALLBK_SORTCB_APP)
BBS_CLEAN_TARGETS += CALLBK_APP_CLEAN

