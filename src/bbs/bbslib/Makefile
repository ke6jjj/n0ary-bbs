include ../../include/config.mk
include ../bbs.mk

PROD =

OBJS =	bbsd.o common.o rfc822.o wpd.o userd.o\
		gated.o logd.o tnc.o msgd.o log.o msg.o bidd.o\
		option.o

CFILES =	bbsd.c common.c rfc822.c wpd.c userd.c\
		gated.c logd.c tnc.c msgd.c log.c msg.c\
		bidd.c option.c

HDRS = bbslib.h
SUPPORT = Makefile
PITCH = libbbs.a

install:	$(INCLUDE)/bbslib.h $(LIB)/libbbs.a

$(INCLUDE)/bbslib.h:	bbslib.h
	cp bbslib.h $(INCLUDE)

$(LIB)/libbbs.a:	libbbs.a
	cp libbbs.a $(LIB)

libbbs.a: $(OBJS)
	$(AR) rcv libbbs.a `lorder $(OBJS) | sort`
	ranlib libbbs.a

$(OBJS):	bbslib.h $(INCLUDE)/c_cmmn.h $(INCLUDE)/tools.h
common.o:	$(INCLUDE)/version.h

include ../common.mk
