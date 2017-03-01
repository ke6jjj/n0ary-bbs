# Get the directory in which this Makefile fragment resides.
BBS_ETC_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

BBS_ETC_SRCS := \
	Config Group HlocScript MOTD NoSubject Personal README Reject Route \
	Systems Translate

BBS_ETC_SRCS_ABS := $(patsubst %,$(BBS_ETC_SRCDIR)/%,$(BBS_ETC_SRCS))

BBS_ETC_INSTALL: $(BBS_ETC_SRCS_ABS)
	for cf in $^; do \
		cp $${cf} $(BBS_ETC_DIR)/$${cf}.sample; \
	done
