# Get the directory in which this Makefile fragment resides.
CALLBK_ULS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CALLBK_ULS_OBJDIR := $(BBS_OBJDIR)/callbk_uls

CALLBK_MAKECB_ULS_APP_SRCDIR := $(CALLBK_ULS_SRCDIR)
CALLBK_MAKECB_ULS_APP_OBJDIR := $(CALLBK_ULS_OBJDIR)

CALLBK_MAKECB_ULS_APP_PROD := makecb_uls

CALLBK_MAKECB_ULS_APP_SRCS := \
	am_database.c call_id.c makecb_uls.c slab.c

#
# We need to see "callbk.h" from the BBS to see what size records
# it is expecting. (As a safeguard).
#
CALLBK_MAKECB_ULS_APP_CFLAGS := -I$(CALLBK_ULS_SRCDIR)/../bbs

$(eval $(call create_bbs_app_rules,CALLBK_MAKECB_ULS_APP))

BBS_PRODUCTS += $(CALLBK_MAKECB_ULS_APP)
BBS_CLEAN_TARGETS += CALLBK_MAKECB_ULS_APP_CLEAN
BBS_INSTALL_TARGETS += CALLBK_MAKECB_ULS_APP_INSTALL
