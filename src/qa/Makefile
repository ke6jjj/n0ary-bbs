THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

all: QA_PRODUCTS

include $(THIS_SRCDIR)/../top.mk

QA_PRODUCTS: $(QA_PRODUCTS)

clean: QA_CLEAN
install: QA_INSTALL
