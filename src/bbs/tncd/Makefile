THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: TNC_APPS

include $(THIS_SRCDIR)/../../top.mk

TNC_APPS: $(TNCD_APP) $(TNCLOG_APP)

clean: TNCD_APP_CLEAN TNCLOG_APP_CLEAN
install: TNCD_APP_INSTALL TNCLOG_APP_INSTALL
