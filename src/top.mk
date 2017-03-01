#
# Get the directory in which this Makefile fragment resides.
#
TOP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TOP_OBJDIR := $(TOP_SRCDIR)/obj

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
include $(TOP_SRCDIR)/qa/qa.mk
include $(TOP_SRCDIR)/bbs/bbs_gmake.mk

#
# Change these as the makefile conversion progresses.
#
BBS_ALL: $(QA_PRODUCTS) $(BBS_PRODUCTS)

install: QA_INSTALL

TOP_CLEAN: TOOLS_CLEAN QA_CLEAN BBS_CLEAN
