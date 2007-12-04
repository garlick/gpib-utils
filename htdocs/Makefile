HTMLMAN= man1/hp8656.1.html man1/hp1630.1.html man1/hp3457.1.html \
	 man1/hptopbm.1.html man1/hptodino.1.html man1/ics8065.1.html \
	 man1/sr620.1.html man1/dg535.1.html man1/a6032.1.html man1/hp3488.1.html
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

man1/%.1.html : %.1
	man2html -r <$< >$@

install: 
	scp * shell.sourceforge.net:/home/groups/g/gp/gpib-utils/htdocs
