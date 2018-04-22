#
# Get the directory in which this Makefile fragment resides.
#
TOP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

ifdef BBS_DEBUG
    TOP_OBJDIR := $(TOP_SRCDIR)/obj-debug
else
    TOP_OBJDIR := $(TOP_SRCDIR)/obj
endif

#
# Get the build settings.
#
include $(TOP_SRCDIR)/include/config.mk

#
# Get the build macros.
#
include $(TOP_SRCDIR)/mk/obj.mk

#
# Get the various component rules.
#
include $(TOP_SRCDIR)/tools/tools.mk
include $(TOP_SRCDIR)/tools/test/tools-test.mk
include $(TOP_SRCDIR)/qa/qa.mk
include $(TOP_SRCDIR)/bbs/bbs_gmake.mk
include $(TOP_SRCDIR)/scripts/scripts.mk

#
# Change these as the makefile conversion progresses.
#
TOP_ALL: $(QA_PRODUCTS) $(BBS_PRODUCTS)

TOP_INSTALL: QA_INSTALL BBS_INSTALL SCRIPTS_INSTALL

TOP_CLEAN: TOOLS_CLEAN QA_CLEAN BBS_CLEAN
