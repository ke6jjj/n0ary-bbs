THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: BBSTOOL_APP

include $(THIS_SRCDIR)/../../top.mk

ifneq ($(ENABLE_BBSTOOL),0)
BBSTOOL_APP: $(BBSTOOL_APP)
else
BBSTOOL_APP:
	$(error "BBS Tool not enabled")
endif

clean: BBSTOOL_APP_CLEAN
install: BBSTOOL_APP_INSTALL
