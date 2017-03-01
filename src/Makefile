#
# Get the directory in which this Makefile fragment resides.
#
THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: TOP_ALL

include $(THIS_SRCDIR)/top.mk

clean: TOP_CLEAN
install: TOP_INSTALL
