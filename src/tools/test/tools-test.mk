#
# Get the directory in which this Makefile fragment resides.
#
TOOLS_TEST_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TOOLS_TEST_OBJDIR := $(TOP_OBJDIR)/tools/test

TOOLS_TEST_SRCS := \
	main.c \
	test.c

TOOLS_TEST_APP := $(TOOLS_TEST_OBJDIR)/test

TOOLS_TEST_CFLAGS := -I$(TOOLS_SRCDIR) -g

$(eval $(call create_compile_rules,TOOLS_TEST))

$(TOOLS_TEST_APP): $(TOOLS_TEST_OBJS) $(TOOLS_DEBUG_LIB)
	$(CC) -o $@ $^

TOOLS_TEST_CLEAN:
	rm -rf $(TOOLS_TEST_OBJDIR)
