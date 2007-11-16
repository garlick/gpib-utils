HTMLMAN= hp8656.html hp1630.html hp3455.html hp3457.html r5005.html \
	 hptopbm.html hptodino.html
VPATH=	../man
OTHER= INSTALL screen.gif

# note: dinotrace.gif is a cropped screen dump taken manually
# note: filter.gif requires manual editing of filter.gp in ../examples/scripts

all: $(HTMLMAN) $(OTHER)

INSTALL: ../INSTALL
	cp $< $@
screen.gif: ../examples/hp1630/screen.gif
	cp $< $@

clean:
	rm -f $(HTMLMAN) $(OTHER)

%.html : %.1
	man2html -r <$< >$@

install: 
	scp * shell.sourceforge.net:/home/groups/g/gp/gpib-utils/htdocs
