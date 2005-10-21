HTMLMAN= hp1630.html hp3455.html hp3457.html r5005.html hptopbm.html
VPATH=	../man
OTHER= INSTALL screen.gif

# note: dinotrace.gif is a cropped screen dump taken manually
# note: sample.gif requires editing of sample.gp in ../examples/hp3457

all: $(HTMLMAN) $(OTHER)

INSTALL: ../INSTALL
	cp $< $@
screen.gif: ../examples/hp1630/screen.gif
	cp $< $@

clean:
	rm -f $(HTMLMAN) $(OTHER)

%.html : %.1
	man2html -r <$< >$@
