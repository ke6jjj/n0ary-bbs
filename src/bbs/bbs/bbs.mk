# Get the directory in which this Makefile fragment resides.
BBS_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BBS_APP_OBJDIR := $(BBS_OBJDIR)/bbs

BBS_APP_PROD := b_bbs

BBS_APP_SRCS := \
	bbs.c body.c\
	callbk.c cmd_pend.c common.c config.c\
	distrib.c\
	event.c \
	file.c filesys.c \
	help.c history.c io.c \
	login.c load.c\
	maintenance.c message.c msg_addr.c msg_body.c msg_edit.c \
	msg_fwd.c msg_fwddir.c msg_gen.c msg_group.c msg_hold.c msg_immune.c\
	msg_kill.c msg_list.c msg_mail.c msg_nts.c \
	msg_read.c msg_send.c msg_trans.c msg_util.c \
	parse.c process.c\
	remote.c rfc822.c\
	search.c server.c system.c\
	tokens.c unixlogin.c\
	user.c usr_disp.c usr_parse.c\
	voice.c\
	wp.c wx.c

$(eval $(call create_bbs_app_rules,BBS_APP))

BBS_PRODUCTS += $(BBS_APP)
BBS_CLEAN_TARGETS += BBS_APP_CLEAN
BBS_INSTALL_TARGETS += BBS_APP_INSTALL
