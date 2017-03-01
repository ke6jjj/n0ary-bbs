include ../include/config.mk

OBJS =	bugs.o daemon.o dectalk.o socket.o sola.o tty.o textline.o\
		smtp_msg.o smtp_send.o smtp_recv.o smtp_log.o\
		common.o error.o host.o spool.o display.o procs.o modem.o\
		strptime.o

CFILES =	bugs.c daemon.c dectalk.c socket.c sola.c tty.c textline.c\
		smtp_msg.c smtp_send.c smtp_recv.c smtp_log.c procs.c\
		common.c error.c host.c spool.c display.c modem.c\
		strptime.c

SRCS = $(CFILES) Makefile smtp.h tools.h dectalk.h c_cmmn.h sola.h

all:	headers archive

archive:	$(LIB)/libtools.a

shared:		$(LIB)/libtools.so.1.0

tar:
	tar cf ../common.tar $(SRCS)

link:
	for file in $(SRCS); do \
		ln -s $(SRC_DIR)/$$file $(DEST_DIR); \
	done

saber:	$(CFILES)
	#load -C $(CFLAGS) -I$(INCLUDE) -DSABER $(CFILES)

headers:	$(INCLUDE)/smtp.h $(INCLUDE)/dectalk.h  $(INCLUDE)/sola.h $(INCLUDE)/tools.h $(INCLUDE)/c_cmmn.h

$(INCLUDE)/smtp.h:	smtp.h
	cp smtp.h $(INCLUDE)

$(INCLUDE)/dectalk.h:	dectalk.h
	cp dectalk.h $(INCLUDE)

$(INCLUDE)/sola.h:	sola.h
	cp sola.h $(INCLUDE)

$(INCLUDE)/tools.h:	tools.h
	cp tools.h $(INCLUDE)

$(INCLUDE)/c_cmmn.h:	c_cmmn.h
	cp c_cmmn.h $(INCLUDE)

$(LIB)/libtools.a:	libtools.a
	cp libtools.a $(LIB)

$(LIB)/libtools.so.1.0:	libtools.so.1.0
	cp libtools.so.1.0 $(LIB)

libtools.so.1.0: $(OBJS)
	$(LD) -o libtools.so.1.0 -assert pure-text $(OBJS)	

libtools.a: $(OBJS)
	$(AR) rcv libtools.a `lorder $(OBJS) | sort`
	ranlib libtools.a

$(OBJS):	$(INCLUDE)/tools.h $(INCLUDE)/c_cmmn.h

smtp_msg.o smtp_send.o smtp_recv.o smtp_log.o:	$(INCLUDE)/smtp.h
dectalk.o:	$(INCLUDE)/dectalk.h
sola.o:	$(INCLUDE)/sola.h

clean:
	rm -f *.o libtools.a *~ TAGS

