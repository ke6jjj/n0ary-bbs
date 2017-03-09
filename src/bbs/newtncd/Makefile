THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: NEWTNC_APP

BUILD_NEWTNCD=1

include $(THIS_SRCDIR)/../../top.mk

NEWTNC_APP: $(NEWTNCD_APP)

clean: NEWTNCD_APP_CLEAN
install: NEWTNCD_APP_INSTALL
