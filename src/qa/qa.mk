#
# Get the directory in which this Makefile fragment resides.
#
QA_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
QA_OBJDIR := $(TOP_OBJDIR)/qa

include include/config.mk

#
# QA executable rules.
#
QA_SRCS := qa.c

QA_CFLAGS := -I$(QA_SRCDIR) -I$(TOOLS_SRCDIR) $(BBS_CFLAGS)

$(eval $(call create_compile_rules,QA))

#
# Conduit executable rules.
#
CONDUIT_SRCDIR := $(QA_SRCDIR)
CONDUIT_OBJDIR := $(QA_OBJDIR)

CONDUIT_SRCS := conduit.c

CONDUIT_CFLAGS := -I$(QA_SRCDIR) -I$(TOOLS_SRCDIR) $(BBS_CFLAGS)

$(eval $(call create_compile_rules,CONDUIT))

QA_APP      := $(QA_OBJDIR)/qa
CONDUIT_APP := $(QA_OBJDIR)/conduit

QA_PRODUCTS := $(QA_APP) $(CONDUIT_APP)

$(QA_APP): $(QA_OBJS) $(TOOLS_LIB)
	$(CC) -o $@ $^ $(QA_LDFLAGS)

$(CONDUIT_APP): $(CONDUIT_OBJS) $(TOOLS_LIB)
	$(CC) -o $@ $^ $(QA_LDFLAGS)

QA_CLEAN:
	rm -rf $(QA_OBJDIR)

QA_INSTALL: $(QA_PRODUCTS)
	install -d $(BBS_DIR)/bin
	install $(QA_APP) $(BBS_DIR)/bin
	install $(CONDUIT_APP) $(BBS_DIR)/bin
