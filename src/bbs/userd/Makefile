include ../../include/config.mk
include ../bbs.mk

PROD = b_userd
OBJS =	main.o usrdir.o parse.o usrfile.o edit.o age.o disp.o
CFILES  =	main.c usrdir.c parse.c usrfile.c edit.c age.c disp.c
HDRS = user.h
SUPPORT = Makefile

all:	$(PROD)

$(OBJS):	$(HDRS)\
			$(INCLUDE)/c_cmmn.h $(INCLUDE)/tools.h $(INCLUDE)/bbslib.h

include ../common.mk
