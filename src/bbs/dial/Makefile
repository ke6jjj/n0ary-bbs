THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: DIAL_APP
clean: DIAL_APP_CLEAN

include $(THIS_SRCDIR)/../../top.mk

DIAL_APP: $(DIAL_APP) $(DIALBS_APP)
