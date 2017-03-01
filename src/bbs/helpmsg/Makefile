include ../../include/config.mk
include ../bbs.mk

PROD = helpmsg
CFILES = helpmsg.c
OBJS = helpmsg.o
HDRS =
SUPPORT = Makefile helpmsg.txt

LOCAL_INC = -I../bbs
ALL: helpmsg $(INS)/../etc/helpmsg.dat $(INS)/../etc/helpmsg.idx

$(INS)/../etc/helpmsg.dat:	helpmsg.dat
	chmod 777 helpmsg.dat
	cp helpmsg.dat $(INS)/../etc

$(INS)/../etc/helpmsg.idx:	helpmsg.idx
	chmod 777 helpmsg.idx
	cp helpmsg.idx $(INS)/../etc

helpmsg.dat:	helpmsg.txt ../bbs/help.h
	helpmsg

include ../common.mk
