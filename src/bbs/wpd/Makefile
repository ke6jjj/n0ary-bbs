include ../../include/config.mk
include ../bbs.mk

PROD = b_wpd
SRC_DIR = bbs/wpd

OBJS =	main.o read.o hash.o date.o parse.o report.o write.o\
		edit.o mail.o update.o search.o upload.o
CFILES =	main.c read.c hash.c date.c parse.c report.c write.c\
		edit.c mail.c update.c search.c upload.c
HDRS = wp.h
SUPPORT = Makefile

include ../common.mk

$(OBJS):	wp.h $(INCLUDE)/c_cmmn.h $(INCLUDE)/tools.h $(INCLUDE)/bbslib.h


