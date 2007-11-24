SUBDIRS=hp1630 hp3457 scripts

all clean install:
	for dir in $(SUBDIRS); do \
	  make -C $$dir $@; \
	done
