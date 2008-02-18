CFLAGS=-g -Wall -I..
LDADD=-L.. -lvxi11 -lrpcsvc -lm

PROGS = 1 2 3 4 5 6 7 8 9 10

all: $(PROGS)


$(PROGS): $(OBJS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $@.c $(OBJS) $(LDADD)

check:
	./runtests $(PROGS)

clean:
	rm -f $(PROGS) *.o
