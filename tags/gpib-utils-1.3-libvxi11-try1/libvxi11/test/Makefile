CFLAGS=-g -Wall -I..
LDADD=-lrpcsvc -lm
OBJS=../vxi11.o ../vxi11_xdr.o ../vxi11_clnt.o ../vxi11intr_xdr.o ../vxi11intr_svc.o

PROGS = 1 2 3 4 5

all: $(PROGS)

$(PROGS): $(OBJS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $@.c $(OBJS) $(LDADD)

clean:
	rm -f $(PROGS) *.o
