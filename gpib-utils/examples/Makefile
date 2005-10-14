SUBDIRS=hp1630 hp3457

all clean install:
	for dir in $(SUBDIRS); do \
	  make -C $$dir $@; \
	done
