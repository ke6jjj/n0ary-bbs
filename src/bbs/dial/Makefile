THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: DIAL_APP

include $(THIS_SRCDIR)/../../top.mk

DIAL_APP: $(DIAL_APP) $(DIALBS_APP)

clean: DIAL_APP_CLEAN
install: DIAL_APP_INSTALL DIALBS_APP_INSTALL
