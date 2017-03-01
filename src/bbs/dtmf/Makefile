THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: DTMF_APP

include $(THIS_SRCDIR)/../../top.mk

ifneq ($(ENABLE_DECTALK),0)
  DTMF_APP: $(DTMF_APP)
else
  DTMF_APP:
	$(error "dectalk not enabled")
endif

clean: DTMF_APP_CLEAN
install: DTMF_APP_INSTALL
