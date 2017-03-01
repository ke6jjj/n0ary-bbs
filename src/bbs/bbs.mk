# Installation location
BBS_ROOT = $(SRC_ROOT)/bbs
TEST_ROOT = $(BBS_ROOT)/TEST
INS_ROOT = $(SRC_ROOT)

INS = $(INS_ROOT)/bin

LOCAL_INC = -I$(BBS_ROOT)
LOCAL_LIBS = -lbbs

SRCS = $(CFILES) $(HDRS) $(SUPPORT)

