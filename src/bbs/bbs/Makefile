include ../../include/config.mk
include ../bbs.mk

PROD = b_bbs

CFILES =\
	bbs.c body.c\
	callbk.c cmd_pend.c common.c config.c\
	distrib.c\
	event.c \
	file.c filesys.c \
	help.c history.c\
	login.c load.c\
	maintenance.c message.c msg_addr.c msg_body.c msg_edit.c \
	msg_fwd.c msg_fwddir.c msg_gen.c msg_group.c msg_hold.c msg_immune.c\
	msg_kill.c msg_list.c msg_mail.c msg_nts.c \
	msg_read.c msg_send.c msg_trans.c msg_util.c \
	parse.c process.c\
	remote.c rfc822.c\
	search.c server.c system.c\
	tokens.c\
	user.c usr_disp.c usr_parse.c\
	voice.c\
	wp.c wx.c

STANDALONE =

HDRS = body.h \
	callbk.h \
	event.h \
	file.h filesys.h function.h \
	help.h history.h \
	message.h msg_fwddir.h \
	vars.h \
	parsetbl.h pending.h process.h \
	rfc822.h \
	server.h system.h \
	tokens.h \
	user.h \
	wp.h wx.h

SUPPORT = Makefile depend.c

OBJS =\
	bbs.o body.o\
	callbk.o cmd_pend.o common.o config.o\
	distrib.o \
	event.o \
	file.o filesys.o \
	help.o history.o \
	login.o load.o \
	maintenance.o message.o msg_addr.o msg_body.o msg_edit.o\
	msg_fwd.o msg_fwddir.o msg_gen.o msg_group.o msg_hold.o msg_immune.o \
	msg_kill.o msg_list.o msg_mail.o msg_nts.o \
	msg_read.o msg_send.o msg_trans.o msg_util.o \
	parse.o process.o \
	remote.o rfc822.o \
	search.o server.o system.o \
	tokens.o \
	user.o usr_disp.o usr_parse.o \
	voice.o \
	wp.o wx.o

ALL:	$(PROD)

depend:	obj.depend
obj.depend:	$(CFILES)
	touch depend.tmp
	makedepend -f depend.tmp $(CFLAGS) $(CFILES)
	cp depend.tmp obj.depend
	rm -f depend.tmp

#include obj.depend
include ../common.mk
