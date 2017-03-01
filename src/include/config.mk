BBS_CONFIG_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

include $(BBS_CONFIG_SRCDIR)/site_config.mk

BBS_CFLAGS := -O2 -Werror -I$(TOP_SRCDIR)/include -DBBS_DIR=\"$(BBS_DIR)\"

ifneq ($(ENABLE_DECTALK),0)
  BBS_CFLAGS += -DDECTALK
endif

ifneq ($(ENABLED_DTMF),0)
  BBS_CFLAGS += -DDTMF
endif

ifdef SUNOS
  BBS_CFLAGS += -DSUNOS
  HAVE_TIMEGM=1
endif
ifdef FREEBSD
  HAVE_TIMEGM=1
  HAVE_TERMIOS=1
  BBS_LDFLAGS+= -lcompat
endif

ifdef HAVE_TIMEGM
  BBS_CFLAGS += -DHAVE_TIMEGM
endif
ifdef HAVE_TERMIOS
  BBS_CFLAGS += -DHAVE_TERMIOS
endif
