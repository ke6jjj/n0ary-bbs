THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: GATED_APP

include $(THIS_SRCDIR)/../../top.mk

GATED_APP: $(GATED_APP)

clean: GATED_APP_CLEAN
install: GATED_APP_INSTALL
