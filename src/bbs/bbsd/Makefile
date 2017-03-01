include ../../include/config.mk
include ../bbs.mk

PROD = b_bbsd
SRC_DIR = bbs/bbsd

OBJS =	main.o parse.o config.o lock.o daemon.o
CFILES =	main.c parse.c config.c lock.c daemon.c
HDRS = bbsd.h
SUPPORT = Makefile

include ../common.mk

$(OBJS):	bbsd.h $(INCLUDE)/tools.h $(INCLUDE)/common.h \
		$(INCLUDE)/c_cmmn.h $(INCLUDE)/config.h

