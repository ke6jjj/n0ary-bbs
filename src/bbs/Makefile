
include ./bbs.mk

# Indicate here what directories are to be built as part
# of your installation:
#
#	common			; required for all builds
#	bbsd bidd gated logd	; all the daemons
#	   tncd userd wpd
#	bbstool			; only for X installations
#	bbs
#	dial gateway		; used if uucp available
#	wx			; weather, only N0ARY
#	dectalk dtmf system	; if you have speech synth
#	metcon sola		; auto shutdown/ups
#	time			; update system clock from Colorado

# make sure that common is first and bbsd is second

DIRS = bbslib bbs bbsd bidd gated logd tcpd tncd userd wpd\
		msgd gateway process dial helpmsg callbk bbstool
#		wx dectalk dtmf system metcon sola time tools

TARGET = 
TEST =

SRCS = Makefile common.mk bbs.mk

all:
	for i in $(DIRS); do\
		echo "======= making $$i ============";\
		cd $$i; make -e $(TARGET); cd ..;\
	done

install:
	make TARGET=install

clean:
	rm -f *~ core
	make TARGET=clean

tar:
	make TARGET=tar

diffs: sccs_diffs
	make TARGET=sccs_diffs

changed: sccs_changed
	make TARGET=sccs_changed

tag:	sccs_tag
	for i in $(DIRS); do\
		echo "======= making $$i ============";\
		cd $$i; make -e BBS=$(BBS) sccs_tag; cd ..;\
	done

inventory:	mk_inventory
	for i in $(DIRS); do\
		echo "======= making $$i ============";\
		cd $$i; make -e mk_inventory; cd ..;\
	done

dist: 	MANIFEST
	make TARGET=tar DIRS="`cat MANIFEST`"
	ls -l *.tar
	compress *.tar
	tar cf SRC.tar *.tar.Z $(SRCS) MANIFEST
	rm -f *.tar.Z
	uuencode SRC.tar SRC.tar > SRC.tar.uu
	ls -l SRC.*

unpack:
	uncompress *.Z
	for i in `cat MANIFEST`; do\
# test for existance of dir before making
		mkdir $$i;\
		cd $$i; tar xf ../$$i.tar;\
		cd ..;\
	done
	rm -f *.tar

MANIFEST:
	+echo "You must create a MANIFEST that contains the directories"
	+echo "you wish to distribute."


#include common.mk

link:
	for file in $(SRCS); do \
		ln -s $(SRC_DIR)/$$file $(DEST_DIR)/$$file;\
	done
	for dir in $(DIRS); do \
		[ -d $(DEST_DIR)/$$dir ] || mkdir $(DEST_DIR)/$$dir; \
	done
	for dir in $(DIRS); do \
		cd $$dir; \
		make DEST_DIR=$(DEST_DIR)/$$dir SRC_DIR=$(SRC_DIR)/$$dir link;\
		cd ..; \
	done

