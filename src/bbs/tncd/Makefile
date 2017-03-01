include ../../include/config.mk
include ../bbs.mk

PROD = b_tncd
OBJS =	ax25.o ax25dump.o ax25subr.o ax25user.o ax_mbx.o\
		bsd_io.o\
		kiss.o\
		lapb.o lapbtime.o\
		main.o mbuf.o monitor.o\
		slip.o socket.o\
		timer.o
CFILES =	ax25.c ax25dump.c ax25subr.c ax25user.c ax_mbx.c\
		bsd_io.c\
		kiss.c\
		lapb.c lapbtime.c\
		main.c mbuf.c monitor.c\
		slip.c socket.c\
		timer.c

HDRS = asy.h ax25.h ax_mbx.h \
		bsd.h \
		function.h \
		global.h \
		kiss.h \
		mbuf.h monitor.h \
		session.h slip.h socket.h \
		timer.h tnc.h trace.h \
		unix.h

SUPPORT = tnclog.c Makefile
PITCH = tnclog

include ../common.mk

tnclog: tnclog.c $(LIB)/libbbs.a $(LIB)/libtools.a
	$(CC) $(CFLAGS) -o tnclog tnclog.c $(LIBS)

