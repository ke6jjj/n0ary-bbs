# Get the directory in which this Makefile fragment resides.
TCPD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TCPD_APP_OBJDIR := $(BBS_OBJDIR)/tcpd

TCPD_APP_PROD := b_tcpd

TCPD_APP_SRCS := main.c parse.c

$(eval $(call create_bbs_app_rules,TCPD_APP))

BBS_PRODUCTS += $(TCPD_APP)
BBS_CLEAN_TARGETS += TCPD_APP_CLEAN
BBS_INSTALL_TARGETS += TCPD_APP_INSTALL
