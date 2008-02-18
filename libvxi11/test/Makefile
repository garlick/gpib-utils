CFLAGS=-g -Wall -I..
LDADD=-L.. -lvxi11

PROGS = 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16  17

all: $(PROGS)

$(PROGS): $(OBJS) ../libvxi11.a

%: %.c
	$(CC) $(CFLAGS) -o $@ $@.c $(OBJS) $(LDADD)

check: $(PROGS)
	./runtests $(PROGS)
checklist:
	@for i in $(PROGS); do printf "%-2.2d: " "$$i";  ./$$i x; done

clean:
	rm -f $(PROGS) *.o
