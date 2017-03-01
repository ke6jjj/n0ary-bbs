include ../../include/config.mk
include ../bbs.mk

PROD = 
CFILES = dialin.c
HDRS =
OBJS = dialin.o
SUPPORT = Makefile

ALL:	b_dialin

b_dialin:	$(OBJS) $(LIB)/libbbs.a $(LIB)/libtools.a
	$(CC) $(CFLAGS) -o b_dialin $(CFILES) $(LIBS) $(LDLIBS)
	$(CC) $(CFLAGS) -DBS -o b_dialinBS $(CFILES) $(LIBS) $(LDLIBS)

include ../common.mk
