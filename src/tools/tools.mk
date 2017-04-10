#
# Get the directory in which this Makefile fragment resides.
#
TOOLS_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TOOLS_OBJDIR := $(TOP_OBJDIR)/tools

TOOLS_SRCS :=	alEvent.c bugs.c daemon.c dectalk.c socket.c sola.c tty.c\
		textline.c smtp_msg.c smtp_send.c smtp_recv.c smtp_log.c\
		procs.c common.c error.c host.c spool.c display.c modem.c\
		safegets.c

TOOLS_CFLAGS := -I$(TOOLS_SRCDIR) $(BBS_CFLAGS)

TOOLS_LIB := $(TOOLS_OBJDIR)/libtools.a

$(eval $(call create_compile_rules,TOOLS))

$(TOOLS_LIB): $(TOOLS_OBJS)
	$(AR) rcv $@ $^

# Make a debugging version of the library available for testing
TOOLS_DEBUG_SRCDIR := $(TOOLS_SRCDIR)
TOOLS_DEBUG_OBJDIR := $(TOOLS_OBJDIR)/debug
TOOLS_DEBUG_SRCS   := $(TOOLS_SRCS)
TOOLS_DEBUG_CFLAGS := -I$(TOOLS_SRCDIR) $(BBS_O_CFLAGS) -g
TOOLS_DEBUG_LIB    := $(TOOLS_DEBUG_OBJDIR)/libtools-debug.a

$(eval $(call create_compile_rules,TOOLS_DEBUG))

$(TOOLS_DEBUG_LIB): $(TOOLS_DEBUG_OBJS)
	$(AR) rcv $@ $^

TOOLS_CLEAN:
	rm -rf $(TOOLS_OBJDIR)
	rm -rf $(TOOLS_DEBUG_OBJDIR)
