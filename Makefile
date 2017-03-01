
#CWD:sh = pwd
CWD=/mnt/bdev/n0ary.bbs

all:
	cd build; make all

test:
	cd build/bbs/TEST; make

link: mk_live
	[ -d build ] || mkdir build
	echo $(CWD)
	cd src; make DEST_DIR=$(CWD)/build SRC_DIR=$(CWD)/src link
#	cd build/bbs/TEST/etc; \
#	chmod u+w .; \
#	rm -f config config.tmp; \
#	sed "s|#SRC_ROOT#|$(CWD)/build|" < config.orig > config.tmp; \
#	sed "s|#BBS_HOST#|bbshost|" < config.tmp > config; \
#	rm config.tmp

ez:
	[ -d ez ] || mkdir ez
	echo $(CWD)
	cd src; make DEST_DIR=$(CWD)/ez SRC_DIR=$(CWD)/src link
	cd ez/include; rm -f config.mk; mv config.ez.mk config.mk

mk_live:
	[ -d live ] || mkdir live
	[ -d live/bin ] || mkdir live/bin
	[ -d live/etc ] || mkdir live/etc
	[ -d live/user ] || mkdir live/user
	[ -d live/message ] || mkdir live/message
	[ -d live/message/Body ] || mkdir live/message/Body
	[ -d live/message/Forward ] || mkdir live/message/Forward
	[ -d live/wp ] || mkdir live/wp
	[ -d live/log ] || mkdir live/log
	[ -d live/spool ] || mkdir live/spool

clobber:
	rm -rf build
	
clobber-live:
	rm -rf live
	make mk_live
