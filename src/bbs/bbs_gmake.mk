# Get the directory in which this Makefile fragment resides.
BBS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BBS_OBJDIR := $(TOP_OBJDIR)/bbs

#
# Each item that should be built will add itself as a target here.
#
BBS_PRODUCTS :=

#
# Likewise, each item to be cleaned should add its target here.
#
BBS_CLEAN_TARGETS :=
BBS_INSTALL_TARGETS :=

#
# Create a macro which can be used to generate rules for compiling a
# BBS application.
#
define create_bbs_app_rules

$1_CFLAGS += $$(BBS_CFLAGS) -I$$($1_SRCDIR) -I$$(BBSLIB_SRCDIR) \
	-I$$(TOOLS_SRCDIR)

$$(eval $$(call create_compile_rules,$1))

$1 := $$($1_OBJDIR)/$$($1_PROD)

$$($1): $$($1_OBJS) $$(BBSLIB_LIB) $$(TOOLS_LIB)
	$$(CC) -o $$@ $$^ $$(BBS_LDFLAGS) $$($1_LDFLAGS)

$1_CLEAN:
	rm -rf $$($1_OBJDIR)

$1_INSTALL: $$($1)
	install -d $(BBS_DIR)/bin
	install $$($1) $(BBS_DIR)/bin/$$($1_PROD)

endef


# For reference only:
#	common			; required for all builds
#	bbsd bidd gated logd	; all the daemons
#	   tncd userd wpd msgd
#	bbstool			; only for X installations
#	bbs
#	dial gateway		; used if uucp available
#	wx			; weather, only N0ARY
#	dectalk dtmf system	; if you have speech synth
#	metcon sola		; auto shutdown/ups
#	time			; update system clock from Colorado
#
# Each of the following Makefile fragments will consult the build configuration
# themselves to determine if they should be built or not.
#
# DIRS = bbslib bbs bbsd bidd gated logd tcpd tncd userd wpd\
#		msgd gateway process dial helpmsg callbk bbstool
#		wx dectalk dtmf system metcon sola time tools

include $(BBS_SRCDIR)/bbslib/bbslib.mk
include $(BBS_SRCDIR)/bbs/bbs.mk
include $(BBS_SRCDIR)/bbsd/bbsd.mk
include $(BBS_SRCDIR)/bbstool/bbstool.mk
include $(BBS_SRCDIR)/bidd/bidd.mk
# include $(BBS_SRCDIR)/calcload/calcload.mk
include $(BBS_SRCDIR)/callbk/callbk.mk
include $(BBS_SRCDIR)/dectalk/dectalk.mk
include $(BBS_SRCDIR)/dial/dial.mk
include $(BBS_SRCDIR)/etc/etc.mk
include $(BBS_SRCDIR)/gated/gated.mk
# include $(BBS_SRCDIR)/gateway/gateway.mk
include $(BBS_SRCDIR)/logd/logd.mk
include $(BBS_SRCDIR)/userd/userd.mk
include $(BBS_SRCDIR)/tncd/tncd.mk
include $(BBS_SRCDIR)/wpd/wpd.mk

BBS_CLEAN: $(BBS_CLEAN_TARGETS) BBSLIB_CLEAN
BBS_INSTALL: $(BBS_INSTALL_TARGETS)
