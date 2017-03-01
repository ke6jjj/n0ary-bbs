# Get the directory in which this Makefile fragment resides.
HELPMSG_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

HELPMSG_APP_SRCDIR := $(HELPMSG_SRCDIR)
HELPMSG_APP_OBJDIR := $(BBS_OBJDIR)/helpmsg

HELPMSG_APP_PROD := helpmsg

HELPMSG_APP_SRCS := helpmsg.c

HELPMSG_APP_CFLAGS := -I$(HELPMSG_APP_SRCDIR)/../bbs

$(eval $(call create_bbs_app_rules,HELPMSG_APP))

$(HELPMSG_APP_OBJDIR)/helpmsg.dat $(HELPMSG_APP_OBJDIR)/helpmsg.idx: \
  $(HELPMSG_APP) $(HELPMSG_SRCDIR)/helpmsg.txt
	$(HELPMSG_APP) $(HELPMSG_SRCDIR)/helpmsg.txt \
		$(HELPMSG_APP_OBJDIR)/helpmsg.dat \
		$(HELPMSG_APP_OBJDIR)/helpmsg.idx

HELPMSG_INSTALL: $(HELPMSG_APP_OBJDIR)/helpmsg.dat \
	$(HELPMSG_APP_OBJDIR)/helpmsg.idx
	install -d $(BBS_DIR)/etc
	cp $(HELPMSG_APP_OBJDIR)/helpmsg.dat $(BBS_DIR)/etc
	cp $(HELPMSG_APP_OBJDIR)/helpmsg.idx $(BBS_DIR)/etc

BBS_PRODUCTS += $(HELPMSG_APP)
BBS_CLEAN_TARGETS += HELPMSG_APP_CLEAN
BBS_INSTALL_TARGETS += HELPMSG_INSTALL
