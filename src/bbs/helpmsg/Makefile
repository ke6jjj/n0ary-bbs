THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: HELPMSG

include $(THIS_SRCDIR)/../../top.mk

HELPMSG: HELPMSG_MESSAGES

clean: HELPMSG_APP_CLEAN
install: HELPMSG_INSTALL
