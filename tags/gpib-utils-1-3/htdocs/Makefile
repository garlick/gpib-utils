HTMLMAN= hp8656.html hp1630.html hp3457.html hptopbm.html hptodino.html \
	 ics8065.html sr620.html dg535.html a6032.html
VPATH=	../man
OTHER= screen.png filter.png

# note: dinotrace.gif is a cropped screen dump taken manually

all: $(HTMLMAN) $(OTHER)

screen.png: ../examples/hp1630/screen.pbm
	pnmtopng <$< >$@	
filter.png: ../examples/scripts/filter.ppm
	pnmtopng <$< >$@	

clean:
	rm -f $(HTMLMAN) $(OTHER)

%.html : %.1
	man2html -r <$< >$@

install: 
	scp * shell.sourceforge.net:/home/groups/g/gp/gpib-utils/htdocs
