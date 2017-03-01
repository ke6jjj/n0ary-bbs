SCCSDIR = $(ROOT)/sccs

all:	$(PROD)

sccs_diffs:
	sh $(ROOT)/src/tools/mkdiff

sccs_changed:
	rm -f CHANGED
	touch CHANGED
	for i in $(SRCS); do \
		echo $$i; \
		if [ `sccs diffs $$i | wc -l` != "2" ] \
		then \
			echo $$i >> CHANGED; \
		fi \
	done

sccs_tag:
	rm -f .Inv.$(BBS);
	touch .Inv.$(BBS);
	for i in $(SRCS); do \
		echo $$i `sccs get -g $$i` >> .Inv.$(BBS); \
	done

mk_inventory:
	rm -f .Inv
	touch .Inv
	for i in $(SRCS); do \
		echo $$i >> .Inv; \
	done

old_mk_inventory:
	rm -f .Inv* N6QMY KB6MER
	touch .Inv .Inv.N6ZFJ .Inv.N6QMY .Inv.KB6MER
	for i in $(SRCS); do \
		echo $$i >> .Inv; \
		echo $$i `sccs get -g $$i` >> .Inv.N6QMY; \
	done
	cp .Inv.N6QMY .Inv.KB6MER
	cp .Inv.N6QMY .Inv.N6ZFJ

tags:	$(SRCS)
	ctags $(SRCS)

tar:
	tar cf ../$(PROD).tar $(SRCS)

saber:	$(CFILES) $(HDRS)
	#cmode
	#load -C $(CFLAGS) -DSABER $(CFILES) ../../lib/libtools.a ../../lib/libbbs.a

install:	$(INS)/$(PROD)

$(INS)/$(PROD):	$(PROD)
	cp $(PROD) $(INS)/$(PROD)

$(PROD):	$(OBJS) $(LIB)/libbbs.a $(LIB)/libtools.a
	$(CC) $(CFLAGS) -o $(PROD) $(OBJS) $(LIBS) $(LDLIBS)

clean:
	rm -f $(OBJS) $(PROD) $(PITCH)
	rm -f core tags TAGS
	rm -f *~
	rm -f #*

link:
	for file in $(SRCS); do\
		ln -s $(SRC_DIR)/$$file $(DEST_DIR)/$$file;\
	done

$(OBJS):	$(HDRS) $(INCLUDE)/c_cmmn.h $(INCLUDE)/tools.h $(INCLUDE)/bbslib.h
