include ../../include/config.mk
include ../bbs.mk

LDLIBS = $(LIBS) -L/usr/openwin/lib -lXol -lXt -lX11
LOCAL_INC = -I/usr/openwin/include -I/usr/openwin/include/Xol -I/usr/openwin/include/X11 -I$(SRC_ROOT)/bbs
XTYPE = -DXol

PROD = b_tool
OBJS = tool.o usrwin.o monitor.o
CFILES = tool.c usrwin.c lckwin.c dmnwin.c bbsd.c monitor.c msgwin.c
HDRS = bbstool.h
SUPPORT = Makefile

include ../common.mk
