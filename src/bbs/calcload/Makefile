THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: CALCLOAD_APP
clean: CALCLOAD_APP_CLEAN

include $(THIS_SRCDIR)/../../top.mk

ifneq ($(ENABLE_CALCLOAD),0)
  CALCLOAD_APP: $(CALCLOAD_APP)
else
  CALCLOAD_APP:
	$(error "calcload not enabled")
endif
