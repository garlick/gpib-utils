/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005-2009 Jim Garlick <garlick.jim@gmail.com>

   gpib-utils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   gpib-utils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with gpib-utils; if not, write to the Free Software Foundation, 
   Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

#include "util.h"

extern char *prog;

/* read to expected length or EOF */
int
read_all(int fd, void *buf, int len)
{
    int n;
    int bc = 0;

    do {
        do {
            n = read(fd, (char *)buf + bc, len - bc);
        } while (n < 0 && errno == EINTR);
        if (n > 0)
            bc += n;
    } while (n > 0 && bc < len);

    return bc;
}

int
write_all(int fd, void *buf, int len)
{
    int n;
    int bc = 0;

    do {
        do {
            n = write(fd, (char *)buf + bc, len - bc);
        } while (n < 0 && errno == EINTR);
        if (n > 0)
            bc += n;
    } while (n > 0 && bc < len);

    return bc;
}

static void _zap_trailing_whitespace(char *s)
{
    char *p = s + strlen(s) - 1;

    while (p >= s && isspace(*p))
        *p-- = '\0';
}

char *
xreadline(char *prompt, char *buf, int buflen)
{
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, buflen, stdin) == NULL)
        return NULL;
    _zap_trailing_whitespace(buf);
    return buf;
}

void *
xmalloc(size_t size)
{
    void *new = malloc(size);

    if (!new) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    return new;
}

void *
xrealloc(void *ptr, int size)
{
    void *new = realloc(ptr, size);

    if (!new) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    return new;
}

char *
xstrdup(char *str)
{
    char *new = strdup(str);

    if (!new) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    return new;
}

char *
xstrcpyprint(char *str)
{
    char *cpy = xmalloc(strlen(str)*2 + 1);
    int i;

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

char *
xstrcpyunprint(char *str)
{
    char *cpy = xmalloc(strlen(str) + 1);
    int i;
    int esc = 0;

    *cpy = '\0';
    for (i = 0; i < strlen(str); i++) {
        if (esc) {
            switch (str[i]) {
                case 'r':
                    strcat(cpy, "\r");
                    break;
                case 'n':
                    strcat(cpy, "\n");
                    break;
                case '\\':
                    strcat(cpy, "\\");
                    break;
                default:
                    strcat(cpy, "\\");
                    strncat(cpy, &str[i], 1);
                    break;
            }
            esc = 0;
        } else if (str[i] == '\\') {
            esc = 1;
        } else {
            strncat(cpy, &str[i], 1);
        }
    }

    return cpy;
}

char *
findstr(strtab_t *tab, int num)
{
    strtab_t *tp;
    static char tmpbuf[64];
    char *res = tmpbuf;

    (void)sprintf(tmpbuf, "unknown code %d", num);
    for (tp = &tab[0]; tp->str; tp++) {
        if (tp->num == num) {
            res = tp->str;
            break;
        }
    }
    return res;
}

int
rfindstr(strtab_t *tab, char *str)
{
    strtab_t *tp;
    int res = -1;

    for (tp = &tab[0]; tp->str; tp++) {
        if (!strcasecmp(tp->str, str)) {
            res = tp->num;
            break;
        }
    }
    return res;
}

/*
 * lsd_* functions are needed by list.[ch].
 */

void 
lsd_fatal_error(char *file, int line, char *mesg)
{
    fprintf(stderr, "%s: fatal error: %s: %s::%d: %s", 
                    prog, mesg, file, line, strerror(errno));
    exit(1);
}

void *
lsd_nomem_error(char *file, int line, char *mesg)
{
    fprintf(stderr, "%s: out of memory: %s: %s::%d", 
                    prog, mesg, file, line);
    exit(1);
}


double 
dbmtov(double a)
{
    return pow(10, (a-13.0)/20);
}


static int
_vtodb(double *a, double offset)
{
    int result = 0;
    double l = log10(*a);

    /* FIXME: isinf() is apparently not portable (solaris 10, gcc 3.4.3) */
    if (l == EDOM || l == ERANGE || /*isinf(l) || */isnan(l))
        result = -1;
    else
        *a = l*20.0 + offset;

    return result;
}

/* Set *ampl to amplitutde in dBm.
 * Return 0 on success, -1 on failure.
 *
 * See pg 64. of HP-8656A operator manual for conversions used below.
 */
int
amplstr(char *str, double *ampl)
{
    int result = 0;
    char *end;
    double a = strtod(str, &end);

    if (!*end)  /* no units */
        result = -1;
    else if (!strcasecmp(end, "dbm"))
        ;
    else if (!strcasecmp(end, "dbf"))
        a -= 120.0;
    else if (!strcasecmp(end, "dbv"))
        a += 13.0;
    else if (!strcasecmp(end, "dbmv"))
        a -= 47.0;
    else if (!strcasecmp(end, "dbuv"))
        a -= 107.0;
    else if (!strcasecmp(end, "dbemfv"))
        a += 7.0;
    else if (!strcasecmp(end, "dbemfmv"))
        a -= 53.0;
    else if (!strcasecmp(end, "dbemfuv"))
        a -= 113.0;
    else if (!strcasecmp(end, "v"))
        result = _vtodb(&a, 13.0);
    else if (!strcasecmp(end, "mv"))
        result = _vtodb(&a, -47.0);
    else if (!strcasecmp(end, "uv"))
        result = _vtodb(&a, -107.0);
    else if (!strcasecmp(end, "emfv"))
        result = _vtodb(&a, 7.0);
    else if (!strcasecmp(end, "emfmv"))
        result = _vtodb(&a, -53.0);
    else if (!strcasecmp(end, "emfmuv"))
        result = _vtodb(&a, -113.0);
    else  
        result = -1;
    if (result == 0 && ampl != NULL)
        *ampl = a;
    return result;
}

/* Set *freq to frequency in Hz.
 * Return 0 on success, -1 on failure.
 */
int
freqstr(char *str, double *freq)
{
    char *end;
    double f = strtod(str, &end);
    int result = 0;

    if (!*end)  /* no units is an error */
        result = -1;
    else if (!strcasecmp(end, "hz"))
        ;
    else if (!strcasecmp(end, "khz"))
        f *= 1E3;
    else if (!strcasecmp(end, "mhz"))
        f *= 1E6;
    else if (!strcasecmp(end, "ghz"))
        f *= 1E9;
    else if (!strcasecmp(end, "sec") || !strcasecmp(end, "s"))
        f = 1.0/f;
    else if (!strcasecmp(end, "msec") || !strcasecmp(end, "ms"))
        f = 1E3/f;
    else if (!strcasecmp(end, "usec") || !strcasecmp(end, "us"))
        f = 1E6/f;
    else if (!strcasecmp(end, "nsec") || !strcasecmp(end, "ns"))
        f = 1E9/f;
    else if (!strcasecmp(end, "psec") || !strcasecmp(end, "ps"))
        f = 1E12/f;
    else  
        result = -1;

    if (result == 0 && freq != NULL)
        *freq = f;

    return result;
}

double
gettime(void)
{
    struct timeval t;

    if (gettimeofday(&t, NULL) < 0) {
        perror("gettimeofday");
        exit(1);
    }
    return ((double)t.tv_sec + (double)t.tv_usec / 1000000);
}

void
sleep_sec(double sec)
{
    if (sec > 0)
        usleep((unsigned long)(sec * 1000000.0));
}

#ifdef FREQMAIN
char *prog = "testfreq";
int
main(int argc, char *argv[])
{
    int i;
    double f;

    for (i = 1; i < argc; i++) {
        if (freqstr(argv[i], &f) < 0)
            printf("error\n");
        else
            printf("%s = %e hz\n", argv[i], f);
    }
    exit(0);
}
#endif

#ifdef AMPLMAIN
char *prog = "testampl";
int
main(int argc, char *argv[])
{
    int i;
    double a;

    for (i = 1; i < argc; i++) {
        if (amplstr(argv[i], &a) < 0)
            printf("error\n");
        else
            printf("%s = %e dBm (%e V)\n", argv[i], a, dbmtov(a));
    }
    exit(0);
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
