# Get the directory in which this Makefile fragment resides.
BBS_ETC_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

BBS_ETC_SRCS := \
	Config Group HlocScript MOTD NoSubject Personal README Reject Route \
	Systems Translate

BBS_ETC_SRCS_ABS := $(patsubst %,$(BBS_ETC_SRCDIR)/%,$(BBS_ETC_SRCS))

BBS_ETC_INSTALL:
	install -d $(BBS_DIR)/etc
	for cf in $(BBS_ETC_SRCS); do \
		cp $(BBS_ETC_SRCDIR)/$${cf} $(BBS_DIR)/etc/$${cf}.sample; \
	done

BBS_INSTALL_TARGETS += BBS_ETC_INSTALL
