# Get the directory in which this Makefile fragment resides.
GATEWAY_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
GATEWAY_APP_OBJDIR := $(BBS_OBJDIR)/gated

GATEWAY_APP_PROD := b_gateway

GATEWAY_APP_SRCS := \
	gateway.c

$(eval $(call create_bbs_app_rules,GATEWAY_APP))

BBS_PRODUCTS += $(GATEWAY_APP)
BBS_CLEAN_TARGETS += GATEWAY_APP_CLEAN
BBS_INSTALL_TARGETS += GATEWAY_APP_INSTALL
