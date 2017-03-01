# Get the directory in which this Makefile fragment resides.
WPD_APP_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
WPD_APP_OBJDIR := $(BBS_OBJDIR)/wpd

WPD_APP_PROD := b_wpd

WPD_APP_SRCS := \
	main.c read.c hash.c date.c parse.c report.c write.c\
		edit.c mail.c update.c search.c upload.c

$(eval $(call create_bbs_app_rules,WPD_APP))

BBS_PRODUCTS += $(WPD_APP)
BBS_CLEAN_TARGETS += WPD_APP_CLEAN
