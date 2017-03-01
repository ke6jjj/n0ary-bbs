include ../include/config.mk

SRCS = qa.c conduit.c Makefile

install:	qa conduit
	cp qa ../bin
	cp conduit ../bin

all: qa conduit

qa:
	$(CC) $(CFLAGS) -o qa qa.c -I../include -L../lib -ltools

conduit:
	$(CC) $(CFLAGS) -o conduit conduit.c -I../include -L../lib -ltools

clean:
	rm -f qa conduit

link:
	for file in $(SRCS); do \
		ln -s $(SRC_DIR)/$$file $(DEST_DIR); \
	done

