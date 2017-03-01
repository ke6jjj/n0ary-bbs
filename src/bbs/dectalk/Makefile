THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: DECTALK_APP

include $(THIS_SRCDIR)/../../top.mk

ifneq ($(ENABLE_DECTALK),0)
  DECTALK_APP: $(DECTALK_APP)
else
  DECTALK_APP:
	$(error "dectalk not enabled")
endif

clean: DECTALK_APP_CLEAN
install: DECTALK_APP_INSTALL
