include ../../include/config.mk
include ../bbs.mk

PROD = b_msgd
OBJS =	main.o read.o rfc822.o list.o parse.o edit.o fwd.o
CFILES =	main.c read.c rfc822.c list.c parse.c edit.c fwd.c
HDRS = msgd.h
SUPPORT = Makefile

all:	$(PROD)

$(OBJS):	msgd.h\
			$(INCLUDE)/c_cmmn.h $(INCLUDE)/tools.h $(INCLUDE)/bbslib.h

include ../common.mk
