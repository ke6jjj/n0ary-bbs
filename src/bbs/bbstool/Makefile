include ../../include/config.mk
include ../bbs.mk

LDLIBS = -L/usr/tools/X/0/lib -lXm -lXt -lX11

PROD = b_tool
OBJS = tool.o usrwin.o lckwin.o dmnwin.o monitor.o msgwin.o
CFILES = tool.c usrwin.c lckwin.c dmnwin.c bbsd.c \
		monitor.c msgwin.c
HDRS = bbstool.h
SUPPORT = Makefile Tool.ad


include ../common.mk
