#define _GNU_SOURCE /* for vasprintf */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <gpib/ib.h>
#include <string.h>

#include "gpib.h"

static char *prog = NULL;
static int verbose = 0;

static char *
strcpyprint(char *str)
{
    char *cpy = malloc(strlen(str)*2 + 1);
    int i;

    if (cpy == NULL) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    *cpy = '\0';
    for (i = 0; i < strlen(str); i++) {
        if (str[i] == '\n')
            strcat(cpy, "\\n");
        else if (str[i] == '\r')
            strcat(cpy, "\\r");
        else
            strncat(cpy, &str[i], 1);
    }
    return cpy;
}

static int
_ibrd(int d, void *buf, int len)
{
    ibrd(d, buf, len);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibrd timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrd error %d\n", prog, iberr);
        exit(1);
    }

    /* XXX This assertion for EOI at the end of a read is commented out
     * because Adam sees it trigger on "hp1630 --save-all", but he still
     * gets complete learn strings.  The reason for the assertion is to
     * document the assumption that 'len' bytes is sufficient to contain
     * the whole instrument response.  Figure out why we are seeing this.
     */
    /*assert(ibsta & END);*/ 
    assert(ibcnt >= 0);
    assert(ibcnt <= len);
   
    return ibcnt;
}

int
gpib_ibrd(int d, void *buf, int len)
{
    int count = _ibrd(d, buf, len);

    if (verbose)
        fprintf(stderr, "R: [%d bytes]\n", count);

    return count;
}

void
gpib_ibrdstr(int d, char *buf, int len)
{
    int count;

    count = _ibrd(d, buf, len - 1);
    assert(count < len);
    buf[count] = '\0';

    if (verbose) {
        char *cpy = strcpyprint(buf);

        fprintf(stderr, "R: \"%s\"\n", cpy);
        free(cpy);
    }
}

static void 
_ibwrt(int d, void *buf, int len)
{
    ibwrt(d, buf, len);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibwrt timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibwrt error %d\n", prog, iberr);
        exit(1);
    }
    assert(ibcnt == len);
}

void
gpib_ibwrt(int d, void *buf, int len)
{
    _ibwrt(d, buf, len);
    if (verbose)
        fprintf(stderr, "T: [%d bytes]\n", len);
}

void 
gpib_ibwrtstr(int d, char *str)
{
    _ibwrt(d, str, strlen(str));
    if (verbose) {
        char *cpy = strcpyprint(str);

        fprintf(stderr, "T: \"%s\"\n", cpy);
        free(cpy);
    }
}

void 
gpib_ibwrtf(int d, char *fmt, ...)
{
        va_list ap;
        char *s;
        int n;

        va_start(ap, fmt);
        n = vasprintf(&s, fmt, ap);
        va_end(ap);
        if (n == -1) {
            fprintf(stderr, "%s: out of memory\n", prog);
            exit(1);
        }
        gpib_ibwrtstr(d, s);
        free(s);
}

void
gpib_ibloc(int d)
{
    ibloc(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibloc error %d\n", prog, iberr);
        exit(1);
    }
    if (verbose)
        fprintf(stderr, "T: [ibloc]\n");
}

void 
gpib_ibclr(int d)
{
    ibclr(d);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibclr timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibclr error %d\n", prog, iberr);
        exit(1);
    }
    if (verbose)
        fprintf(stderr, "T: [ibclr]\n");
}

void
gpib_ibtrg(int d)
{
    ibtrg(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibtrg error %d\n", prog, iberr);
        exit(1);
    }
    if (verbose)
        fprintf(stderr, "T: [ibtrg]\n");
}

void 
gpib_ibrsp(int d, char *status)
{
    ibrsp(d, status); /* NOTE: modifies ibcnt */
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrsp error %d\n", prog, iberr);
        exit(1);
    }
    if (verbose)
        fprintf(stderr, "T: [ibrsp] R: 0x%x\n", (unsigned int)*status);
}

int
gpib_init(char *progname, char *instrument, int vflag)
{
    int d;

    prog = progname;
    verbose = vflag;

    d = ibfind(instrument);
    if (d < 0) {
        fprintf(stderr, "%s: could not find device named %s in gpib.conf\n", 
                prog, instrument);
        exit(1);
    }

    return d;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
