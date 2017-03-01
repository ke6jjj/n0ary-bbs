all: mktools mkqa mkbbs

mktools:
	cd tools; make

mkqa:
	cd qa; make

mkbbs:
	cd bbs; make install

clean:
	cd tools; make clean
	cd qa; make clean
	cd bbs; make clean
	rm lib/*
	rm include/*.h

link:
	[ -d $(DEST_DIR)/tools ] || mkdir $(DEST_DIR)/tools
	[ -d $(DEST_DIR)/qa ] || mkdir $(DEST_DIR)/qa
	[ -d $(DEST_DIR)/bbs ] || mkdir $(DEST_DIR)/bbs
	[ -d $(DEST_DIR)/lib ] || mkdir $(DEST_DIR)/lib
	[ -d $(DEST_DIR)/include ] || mkdir $(DEST_DIR)/include
	[ -d $(DEST_DIR)/bin ] || mkdir $(DEST_DIR)/bin
	cd include; for file in *; do \
		[ -d $$file ] || sed "s|#SRC_ROOT#|$(DEST_DIR)|" < $(SRC_DIR)/include/$$file > $(DEST_DIR)/include/$$file; \
	done
	cd include; rm -f $(DEST_DIR)/include/version.h; ln -s $(SRC_DIR)/include/version.h $(DEST_DIR)/include/version.h
	ln -s $(SRC_DIR)/Makefile $(DEST_DIR)
	cd tools; make DEST_DIR=$(DEST_DIR)/tools SRC_DIR=$(SRC_DIR)/tools link
	cd qa; make DEST_DIR=$(DEST_DIR)/qa SRC_DIR=$(SRC_DIR)/qa link
	cd bbs; make DEST_DIR=$(DEST_DIR)/bbs SRC_DIR=$(SRC_DIR)/bbs link
