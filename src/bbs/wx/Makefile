THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: WX_APPS

include $(THIS_SRCDIR)/../../top.mk

WX_APPS: $(WX_APP)

clean: WX_APP_CLEAN
install: WX_APP_INSTALL
