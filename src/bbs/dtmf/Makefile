include ../bbs.mk

PRODUCTS = dtmf

SRCS = dtmf.c Makefile

dtmf: dtmf.c
	$(CC) $(CFLAGS) -o dtmf dtmf.c $(LIBTOOLS)

tar:
	tar cf ../dtmf.tar $(SRCS)

install:	$(INS)/dtmf

$(INS)/dtmf:	dtmf
	cp dtmf $(INS)

include ../common.mk
