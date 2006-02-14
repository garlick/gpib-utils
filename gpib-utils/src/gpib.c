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
            sprintf(cpy + strlen(cpy), "%c", str[i]);
    }
    return cpy;
}

static int
_ibrd(int d, void *buf, int len)
{
    int count = 0;

    do {
        ibrd(d, buf + count, len - count);
        if (ibsta & TIMO) {
            fprintf(stderr, "%s: ibrd timeout\n", prog);
            exit(1);
        }
        if (ibsta & ERR) {
            fprintf(stderr, "%s: ibrd error %d\n", prog, iberr);
            exit(1);
        }
        count += ibcnt;
        assert(len >= 0);
    } while (!(ibsta & END) && ibcnt > 0 && len - count > 0);
    assert(ibsta & END);
    assert(count >= 0);
    assert(count <= len);
   
    return count;
}

int
gpib_ibrd(int d, void *buf, int len)
{
    int count = _ibrd(d, buf, len);

    if (verbose)
        fprintf(stderr, "R: [%d bytes]\n", count);

    return count;

}

#if 0
static void
_chomp(char *str)
{
    char *p = str + strlen(str) - 1;

    while (*p == '\r' || *p == '\n')
        *p-- = '\0';
}
#endif

void
gpib_ibrdstr(int d, char *buf, int len)
{
    int count;

    count = _ibrd(d, buf, len - 1);
    assert(count < len);
    buf[count] = '\0';
#if 0
    _chomp(buf);
#endif
    if (verbose) {
        char *cpy = strcpyprint(buf);

        fprintf(stderr, "R: \"%s\"\n", cpy);
        free(cpy);
    }
}

static void 
_ibwrt(int d, void *buf, int len)
{
    int count = 0;

    do {
        ibwrt(d, buf + count, len - count);
        if (ibsta & TIMO) {
            fprintf(stderr, "%s: ibwrt timeout\n", prog);
            exit(1);
        }
        if (ibsta & ERR) {
            fprintf(stderr, "%s: ibwrt error %d\n", prog, iberr);
            exit(1);
        }
        count += ibcnt;
    } while (len - count > 0);
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
        fprintf(stderr, "%s: counld not find device named %s in gpib.conf\n", 
                prog, instrument);
        exit(1);
    }

    return d;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
