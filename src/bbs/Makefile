THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: BBS_PRODUCTS

include $(THIS_SRCDIR)/../top.mk

BBS_PRODUCTS: $(BBS_PRODUCTS)

clean: BBS_CLEAN
