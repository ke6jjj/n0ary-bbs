#
# Get the directory in which this Makefile fragment resides.
#
BBSLIB_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BBSLIB_OBJDIR := $(BBS_OBJDIR)/bbslib

BBSLIB_SRCS :=	bbsd.c common.c rfc822.c wpd.c userd.c\
		gated.c logd.c tnc.c msgd.c log.c msg.c\
		bidd.c option.c

BBSLIB_CFLAGS := -I$(BBSLIB_SRCDIR) -I$(TOOLS_SRCDIR) $(BBS_CFLAGS)

$(eval $(call create_compile_rules,BBSLIB))

BBSLIB_LIB := $(BBSLIB_OBJDIR)/libbbs.a

$(BBSLIB_LIB): $(BBSLIB_OBJS)
	$(AR) rcv $@ $^


BBSLIB_CLEAN:
	rm -rf $(BBSLIB_OBJDIR)
