include ../../include/config.mk
include ../bbs.mk

PROD = b_process
OBJS = process.o mail.o import.o command.o read.o
CFILES = process.c mail.c import.c command.c read.c
HDRS =
SUPPORT = Makefile
LOCAL_INC = -I../gateway

include ../common.mk

$(OBJS):	../gateway/gateway.h
