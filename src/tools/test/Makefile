THIS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

run: TOOLS_TEST_APP
	$(TOOLS_TEST_APP)

debug: TOOLS_TEST_APP
	gdb $(TOOLS_TEST_APP)

clean: TOOLS_TEST_CLEAN

include $(THIS_SRCDIR)/../../top.mk

TOOLS_TEST_APP: $(TOOLS_TEST_APP)
