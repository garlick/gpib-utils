CFLAGS=-g -Wall -I..
LDADD=-L.. -lvxi11 -lrpcsvc -lm

PROGS = 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 

all: $(PROGS)


$(PROGS): $(OBJS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $@.c $(OBJS) $(LDADD)

check:
	./runtests $(PROGS)
checklist:
	@for i in $(PROGS); do printf "%-2.2d: " "$$i";  ./$$i x; done

clean:
	rm -f $(PROGS) *.o
