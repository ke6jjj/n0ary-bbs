# root of source tree
SRC_ROOT = /opt/src

# Complier choice
CC = acc
ANSI = -Xt
#CC = gcc
#ANSI = -ansi

ENVIRONMENT = -D_BSD_SOURCE -DSUNOS
INCLUDE = $(SRC_ROOT)/include
LIB = $(SRC_ROOT)/lib

INC = $(LOCAL_INC) -I$(INCLUDE)
LIBS = -L$(SRC_ROOT)/lib $(LOCAL_LIBS) -ltools
CFLAGS = $(DBUG) $(XTYPE) $(INC) $(ANSI) $(ENVIRONMENT)
