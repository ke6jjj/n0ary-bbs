include ../../include/config.mk
include ../bbs.mk

PRODUCTS = make_cb disp_cb sort_cb

OBJS =	make_cb.o

SRCS =	make_cb.c Makefile disp_cb.c sort_cb.c

all:	make_cb disp_cb sort_cb

make_cb: make_cb.c  $(LIB)/libbbs.a $(LIB)/libtools.a
	$(CC) $(CFLAGS) -o make_cb make_cb.c $(LIBS)

disp_cb: disp_cb.c  $(LIB)/libbbs.a $(LIB)/libtools.a
	$(CC) $(CFLAGS) -o disp_cb disp_cb.c $(LIBS)

sort_cb: sort_cb.c  $(LIB)/libbbs.a $(LIB)/libtools.a
	$(CC) $(CFLAGS) -o sort_cb sort_cb.c $(LIBS)

install:

create:	make_cb sort_cb
	make_cb . $(INS)/../callbk
	sort_cb $(INS)/../callbk/lastname.indx
	sort_cb $(INS)/../callbk/firstname.indx
	sort_cb $(INS)/../callbk/city.indx
	sort_cb $(INS)/../callbk/zip.indx

$(INS)/make_cb:	make_cb
	cp make_cb $(INS)/make_cb

tar:
	tar cf ../callbk.tar $(SRCS)

include ../common.mk
