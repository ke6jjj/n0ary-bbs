# root of source tree
SRC_ROOT = /usr2/n0ary.bbs/ez

# Complier choice
CC = lcc -Xez
LD = lcc -Xez
AR = energize_ar

ANSI = -Xa
DBUG = -g

ENVIRONMENT = -D_BSD_SOURCE -DSUNOS -DDECTALK -DDTMF -D_DEBUG
INCLUDE = $(SRC_ROOT)/include
LIB = $(SRC_ROOT)/lib

INC = $(LOCAL_INC) -I$(INCLUDE)
LIBS = -L$(SRC_ROOT)/lib $(LOCAL_LIBS) -ltools
CFLAGS = $(DBUG) $(XTYPE) $(INC) $(ANSI) $(ENVIRONMENT)
