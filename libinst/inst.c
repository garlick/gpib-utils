/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2001-2009 Jim Garlick <garlick.jim@gmail.com>

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#if HAVE_LINUX_GPIB
#include <gpib/ib.h>
#endif
#include <errno.h>
#include <string.h>
#include <ctype.h>
#ifndef __USE_ISOC99
#define __USE_ISOC99 /* activate vsscanf prototype in stdio.h */
#endif
#include <stdio.h>
#include <libgen.h>
#include <termios.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include <wordexp.h>

#if HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef enum { false=0, true=1 } bool;
#endif

#include "libutil/util.h"
#include "libvxi11/vxi11_device.h"
#include "libutil/hprintf.h"

#include "inst.h"

typedef enum { GPIB, VXI11, SERIAL, SOCKET } contype_t;

#define INSTRUMENT_MAGIC 0x43435334
struct instrument {
    int             magic;
    contype_t       contype;   /* type of connection */
    char           *name;      /* instrument name */
    int             verbose;   /* verbose flag (print telemetry on stderr) */
    spollfun_t      sf_fun;    /* app-specific serial poll function */
    int             sf_level;  /* serial poll recursion detection */
    unsigned long   sf_retry;  /* backoff factor for serial poll retry (uS) */
    int             d;         /* handle (GPIB) */
    int             fd;        /* file descriptor (SOCKET, SERIAL) */
    vxi11dev_t      vxi11_handle; /* handle (VXI11) */
    int             reos;
    int             eos;
    int             eot;
    struct timeval  timeout;
};

typedef struct {
    int baud;
    speed_t bconst;
} baudmap_t;

static baudmap_t baudmap[] = {
    {300,   B300},
    {1200,  B1200},
    {2400,  B2400},
    {4800,  B4800},
    {9600,  B9600},
    {19200, B19200},
    {38400, B38400},
#ifdef B57600
    {57600, B57600},
#endif
#ifdef B115200
    {115200,B115200},
#endif
#ifdef B230400
    {230400,B230400},
#endif
#ifdef B460800
    {460800,B460800},
#endif
};

extern char *prog;

static int _raw_serial(struct instrument *gd);
static int _canon_serial(struct instrument *gd);

/* If a serial poll function is defined, call it with the instrument
 * status byte.
 */
static void
_serial_poll(struct instrument *gd, char *str)
{
    unsigned char sb;
    int err = 0;
    int loops = 0;
    int more;

    gd->sf_level++;
    if (gd->sf_level == 1 && gd->sf_fun) {
        /* Multiple status byte values are possible if AUTOPOLL is enabled.
         * (The driver maintains a stack of them.)
         */
        do {
            more = inst_rsp(gd, &sb);
            err = gd->sf_fun(gd, sb, str);
            switch (err) {
                case -1:    /* retry - device not ready (and we care) */
                    usleep(++loops*gd->sf_retry);
                    break;
                case 0:     /* no error */
                    break;
                default:    /* fatal error */
                    inst_fini(gd);
                    exit(err);
            }
        } while (more || err == -1);
    }
    gd->sf_level--;
}

static int
_generic_read(struct instrument *gd, char *buf, int len)
{
    int err, count = 0;

    switch (gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            do {
                ibrd(gd->d, buf + count, len - count);
                if (ibsta & TIMO) {
                    fprintf(stderr, "%s: ibrd timeout\n", prog);
                    inst_fini(gd);
                    exit(1);
                }
                if (ibsta & ERR) {
                    fprintf(stderr, "%s: ibrd error %d\n", prog, iberr);
                    inst_fini(gd);
                    exit(1);
                }
                count += ibcnt;
            } while (count < len && !(ibsta & END));
            if (!(ibsta & END)) {
                fprintf(stderr, "%s: read buffer too small\n", prog);
                inst_fini(gd);
                exit(1);
            }
#endif
            break;
        case VXI11:
            if ((err = vxi11_read(gd->vxi11_handle, buf, len, &count))) {
                vxi11_perror(gd->vxi11_handle, err, prog);
                inst_fini(gd);
                exit(1);
            }
            break;
        case SERIAL:
            /* FIXME: use timeout */
            if (gd->reos)
                count = read(gd->fd, buf, len);
            else
                count = read_all(gd->fd, buf, len);
            if (count < 0) {
                fprintf(stderr, "%s: read error: %s\n", prog, strerror(errno));
                inst_fini(gd);
                exit(1);
            } else if (count == 0) {
                fprintf(stderr, "%s: EOF on read: %s\n", prog, strerror(errno));
                inst_fini(gd);
                exit(1);
            }
            break;
        case SOCKET:
            /* FIXME: use timeout */
            if ((count = read_all(gd->fd, buf, len) < 0)) {
                fprintf(stderr, "%s: read error: %s\n", prog, strerror(errno));
                inst_fini(gd);
                exit(1);
            } else if (count == 0) {
                fprintf(stderr, "%s: EOF on read: %s\n", prog, strerror(errno));
                inst_fini(gd);
                exit(1);
            }
            break;
    }

    return count;
}

int
inst_rd(struct instrument *gd, void *buf, int len)
{
    int count = 0;

    assert(gd->magic == INSTRUMENT_MAGIC);
    count = _generic_read(gd, buf, len);
    if (gd->verbose)
        fprintf(stderr, "R: [%d bytes]\n", count);
    _serial_poll(gd, "gpib_rd");

    return count;
}

static void
_zap_trailing_terminators(char *buf)
{
    char *p = buf + strlen(buf) - 1;

    while (p >= buf && (*p == '\r' || *p == '\n'))
        *p-- = '\0';
}

void
inst_rdstr(struct instrument *gd, char *buf, int len)
{
    int count = 0;

    assert(gd->magic == INSTRUMENT_MAGIC);
    count = _generic_read(gd, buf, len - 1);
    assert(count < len);
    buf[count] = '\0';
    _zap_trailing_terminators(buf);
    if (gd->verbose) {
        char *cpy = xstrcpyprint(buf);

        fprintf(stderr, "R: \"%s\"\n", cpy);
        free(cpy);
    }
    _serial_poll(gd, "gpib_rdstr");
}

int
inst_rdf(struct instrument *gd, char *fmt, ...)
{
    va_list ap;
    char buf[1024];
    int count = 0;
    int n;

    assert(gd->magic == INSTRUMENT_MAGIC);
    count = _generic_read(gd, buf, sizeof(buf) - 1);
    assert(count < sizeof(buf) - 1);
    buf[count] = '\0';
    _zap_trailing_terminators(buf);

    va_start(ap, fmt);
#if HAVE_VSSCANF
    n = vsscanf(buf, fmt, ap);
#else
#error vsscanf is unavailable
#endif
    va_end(ap);

    if (gd->verbose) {
        char *cpy = xstrcpyprint(buf);

        fprintf(stderr, "R: \"%s\"\n", cpy);
        free(cpy);
    }
    _serial_poll(gd, "gpib_rdf");

    return n;
}

static void
_generic_write(struct instrument *gd, void *buf, int len)
{
    int err;

    switch (gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibwrt(gd->d, buf, len);
            if (ibsta & TIMO) {
                fprintf(stderr, "%s: ibwrt timeout\n", prog);
                inst_fini(gd);
                exit(1);
            }
            if (ibsta & ERR) {
                fprintf(stderr, "%s: ibwrt error %d\n", prog, iberr);
                inst_fini(gd);
                exit(1);
            }
            assert(ibcnt == len);
#endif
            break;
        case VXI11:
            if ((err = vxi11_write(gd->vxi11_handle, buf, len))) {
                vxi11_perror(gd->vxi11_handle, err, prog);
                inst_fini(gd);
                exit(1);
            }
            break;
        case SERIAL:
        case SOCKET:
            /* FIXME: use timeout */
            if (write_all(gd->fd, buf, len) < 0) {
                fprintf(stderr, "%s: write error: %s\n", prog, strerror(errno));
                inst_fini(gd);
                exit(1);
            }
            break;
    }
}

void
inst_wrt(struct instrument *gd, void *buf, int len)
{
    assert(gd->magic == INSTRUMENT_MAGIC);
    _generic_write(gd, buf, len);
    if (gd->verbose)
        fprintf(stderr, "T: [%d bytes]\n", len);
    _serial_poll(gd, "gpib_wrt");
}

void
inst_wrtstr(struct instrument *gd, char *str)
{
    assert(gd->magic == INSTRUMENT_MAGIC);
    _generic_write(gd, str, strlen(str));
    if (gd->verbose) {
        char *cpy = xstrcpyprint(str);

        fprintf(stderr, "T: \"%s\"\n", cpy);
        free(cpy);
    }
    _serial_poll(gd, "gpib_wrtstr");
}

void
inst_wrtf(struct instrument *gd, char *fmt, ...)
{
    va_list ap;
    char *s;

    assert(gd->magic == INSTRUMENT_MAGIC);
    va_start(ap, fmt);
    s = hvsprintf(fmt, ap);
    va_end(ap);
    _generic_write(gd, s, strlen(s));
    if (gd->verbose) {
        char *cpy = xstrcpyprint(s);

        fprintf(stderr, "T: \"%s\"\n", cpy);
        free(cpy);
    }
    free(s);
    _serial_poll(gd, "gpib_wrtf");
}

int
inst_qry(struct instrument *gd, char *str, void *buf, int len)
{
    int count;

    assert(gd->magic == INSTRUMENT_MAGIC);
    _generic_write(gd, str, strlen(str));
    if (gd->verbose) {
        char *cpy = xstrcpyprint(str);

        fprintf(stderr, "T: \"%s\"\n", cpy);
        free(cpy);
    }
    count = _generic_read(gd, buf, len);
    if (count < len && ((char *)buf)[count - 1] != '\0')
        ((char *)buf)[count++] = '\0';
    if (gd->verbose) {
        if (((char *)buf)[count - 1] == '\0' && strlen((char *)buf) < 60) {
            char *cpy = xstrcpyprint(buf);

            fprintf(stderr, "R: \"%s\"\n", cpy);
            free(cpy);
        } else  {
            fprintf(stderr, "R: [%d bytes]\n", count);
        }
    }
    _serial_poll(gd, "gpib_qry");

    return count;
}

int
inst_qrystr(struct instrument *gd, char *str, char *buf, int len)
{
    int count;

    count = inst_qry(gd, str, buf, len - 1);
    buf[count > 0 ? count : 0] = '\0';
    _zap_trailing_terminators(buf);
    return count;
}

int
inst_qryint(struct instrument *gd, char *str)
{
    char buf[16];

    inst_qrystr(gd, str, buf, sizeof(buf));

    return strtoul(buf, NULL, 10); /* 0 - 255 */
}

void
inst_loc(struct instrument *gd)
{
    int err;

    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibloc(gd->d);
            if ((ibsta & ERR)) {
                fprintf(stderr, "%s: ibloc error %d\n", prog, iberr);
                inst_fini(gd);
                exit(1);
            }
#endif
            break;
        case VXI11:
            if ((err = vxi11_local(gd->vxi11_handle))) {
                vxi11_perror(gd->vxi11_handle, err, prog);
                exit(1);
            }
            break;
        case SERIAL:
        case SOCKET:
            break;
    }
    if (gd->verbose)
        fprintf(stderr, "T: [ibloc]\n");
    _serial_poll(gd, "gpib_loc");
}

void
inst_clr(struct instrument *gd, unsigned long usec)
{
    int err;

    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibclr(gd->d);
            if ((ibsta & TIMO)) {
                fprintf(stderr, "%s: ibclr timeout\n", prog);
                inst_fini(gd);
                exit(1);
            }
            if ((ibsta & ERR)) {
                fprintf(stderr, "%s: ibclr error %d\n", prog, iberr);
                inst_fini(gd);
                exit(1);
            }
#endif
            break;
        case VXI11:
            if ((err = vxi11_clear(gd->vxi11_handle))) {
                vxi11_perror(gd->vxi11_handle, err, prog);
                exit(1);
            }
            break;
        case SERIAL:
        case SOCKET:
            break;
    }
    if (gd->verbose)
        fprintf(stderr, "T: [ibclr]\n");
    usleep(usec);
    _serial_poll(gd, "gpib_clr");
}

void
inst_trg(struct instrument *gd)
{
    int err;

    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibtrg(gd->d);
            if ((ibsta & ERR)) {
                fprintf(stderr, "%s: ibtrg error %d\n", prog, iberr);
                inst_fini(gd);
                exit(1);
            }
#endif
            break;
        case VXI11:
            if ((err = vxi11_trigger(gd->vxi11_handle))) {
                vxi11_perror(gd->vxi11_handle, err, prog);
                inst_fini(gd);
                exit(1);
            }
        case SERIAL:
        case SOCKET:
            break;
    }
    if (gd->verbose)
        fprintf(stderr, "T: [ibtrg]\n");
    _serial_poll(gd, "gpib_trg");
}

/* A nonzero return value means call gpib_rsp() again to obtain more
 * status info.
 */
int
inst_rsp(struct instrument *gd, unsigned char *status)
{
    int err, res = 0;

    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibrsp(gd->d, (char *)status); /* NOTE: modifies ibcnt */
            if ((ibsta & ERR)) {
                fprintf(stderr, "%s: ibrsp error %d\n", prog, iberr);
                inst_fini(gd);
                exit(1);
            }
            res = (ibsta & RQS);
#endif
            break;
        case VXI11:
            if ((err = vxi11_readstb(gd->vxi11_handle, status))) {
                vxi11_perror(gd->vxi11_handle, err, prog);
                exit(1);
            }
            break;
        case SERIAL:
        case SOCKET:
            /* FIXME */
            *status = 0;
            break;
    }
    if (gd->verbose)
        fprintf(stderr, "T: [ibrsp] R: 0x%x\n", (unsigned int)*status);
    return res;
}

void
inst_set_reos(struct instrument *gd, int flag)
{
    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibconfig(gd->d, IbcEOSrd, flag ? REOS : 0);
            if ((ibsta & ERR)) {
                fprintf(stderr, "%s: ibconfig IbcEOSrd failed: %d\n",
                        prog, iberr);
            }
#endif
            break;
        case VXI11:
            vxi11_set_termcharset(gd->vxi11_handle, flag);
            break;
        case SERIAL:
            gd->reos = flag;
            if (flag)
                _canon_serial(gd);
            else
                _raw_serial(gd);
            break;
        case SOCKET:
            gd->reos = flag;
            break;
    }
}

void
inst_set_eot(struct instrument *gd, int flag)
{
    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibconfig(gd->d, IbcEOT, flag);
            if ((ibsta & ERR)) {
                fprintf(stderr, "%s: ibconfig IbcEOT failed: %d\n",
                    prog, iberr);
            }
#endif
            break;
        case VXI11:
            vxi11_set_endw(gd->vxi11_handle, flag);
            break;
        case SERIAL:
        case SOCKET:
            gd->eot = flag;
            break;
    }
}

void
inst_set_eos(struct instrument *gd, int c)
{
    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibconfig(gd->d, IbcEOSchar, c);
            if ((ibsta & ERR)) {
                fprintf(stderr, "%s: ibconfig IbcEOSchar failed: %d\n",
                        prog, iberr);
            }
#endif
            break;
        case VXI11:
            vxi11_set_termchar(gd->vxi11_handle, c);
            break;
        case SERIAL:
            gd->eos = c;
            if (gd->reos)
                _canon_serial(gd);
            break;
        case SOCKET:
            gd->eos = c;
            break;
    }
}

#if HAVE_LINUX_GPIB
static void
_ibtmo(struct instrument *gd, double sec)
{
    char *str;
    int val = T1000s;

    if (sec < 0 || sec > 1000) {
        fprintf(stderr, "%s: gpib_set_timeout: timeout out of range\n", prog);
        inst_fini(gd);
        exit(1);
    }
    if (sec == 0) {
        val =  TNONE;   str = "TNONE";
    } else if (sec <= 0.00001) {
        val =  T10us;   str = "T10us";
    } else if (sec <= 0.00003) {
        val =  T30us;   str = "T30us";
    } else if (sec <= 0.00010) {
        val =  T100us;  str = "T100us";
    } else if (sec <= 0.00030) {
        val =  T300us;  str = "T300us";
    } else if (sec <= 0.00100) {
        val =  T1ms;    str = "T1ms";
    } else if (sec <= 0.00300) {
        val =  T3ms;    str = "T3ms";
    } else if (sec <= 0.01000) {
        val =  T10ms;   str = "T10ms";
    } else if (sec <= 0.03000) {
        val =  T30ms;   str = "T30ms";
    } else if (sec <= 0.10000) {
        val =  T100ms;  str = "T100ms";
    } else if (sec <= 0.30000) {
        val =  T300ms;  str = "T300ms";
    } else if (sec <= 1.00000) {
        val =  T1s;     str = "T1s";
    } else if (sec <= 3.00000) {
        val =  T3s;     str = "T3s";
    } else if (sec <= 10.00000) {
        val =  T10s;    str = "T10s";
    } else if (sec <= 30.00000) {
        val =  T30s;    str = "T30s";
    } else if (sec <= 100.00000) {
        val =  T100s;   str = "T100s";
    } else if (sec <= 300.00000) {
        val =  T300s;   str = "T300s";
    } else if (sec <= 1000.00000) {
        val =  T1000s;  str = "T1000s";
    }
    ibtmo(gd->d, val);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibtmo failed: %d\n", prog, iberr);
        inst_fini(gd);
        exit(1);
    }
}
#endif

void
inst_set_timeout(struct instrument *gd, double sec)
{
    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            _ibtmo(gd, sec);
#endif
             break;
        case VXI11:
             vxi11_set_iotimeout(gd->vxi11_handle, sec * 1000.0);
             break;
        case SERIAL:
        case SOCKET:
             gd->timeout.tv_sec = (time_t)floor(sec);
             gd->timeout.tv_usec =  (suseconds_t)((sec - floor(sec)) / 1E6);
             break;
    }
}

void
inst_set_verbose(struct instrument *gd, int flag)
{
    assert(gd->magic == INSTRUMENT_MAGIC);
    gd->verbose = flag;
}

static void
_free_inst(struct instrument *gd)
{
    memset(gd, 0, sizeof(*gd));
    free(gd);
}

/* Call to abort in-progress RPC on core channel.
 */
void
inst_abort(struct instrument *gd)
{
    int err;

    switch(gd->contype) {
        case VXI11:
            err = vxi11_abort(gd->vxi11_handle);
            if (err) /* N.B. non-fatal */
                vxi11_perror(gd->vxi11_handle, err, prog);
            break;
        case GPIB:
        case SERIAL:
        case SOCKET:
            break;
    }
}

void
inst_fini(struct instrument *gd)
{
    assert(gd->magic == INSTRUMENT_MAGIC);
    switch(gd->contype) {
        case GPIB:
#if HAVE_LINUX_GPIB
            ibonl(gd->d, 0);
#endif
            break;
        case VXI11:
            if (gd->vxi11_handle) {
                vxi11_close(gd->vxi11_handle);
                vxi11_destroy(gd->vxi11_handle);
                gd->vxi11_handle = NULL;
            }
            break;
        case SERIAL:
        case SOCKET:
            if (gd->fd >= 0) {
                (void)close(gd->fd);
                gd->fd = -1;
            }
            break;
    }
    _free_inst(gd);
}

static struct instrument *
_new_inst(contype_t t)
{
    struct instrument *new = xmalloc(sizeof(*new));

    new->magic = INSTRUMENT_MAGIC;
    new->contype = t;
    new->d = -1;
    new->verbose = 0;
    new->sf_fun = NULL;
    new->sf_level = 0;
    new->sf_retry = 1;
    new->vxi11_handle = NULL;
    new->fd = -1;
    new->reos = 0;
    new->eot = 1;
    new->eos = '\n';
    timerclear(&new->timeout);

    return new;
}

static struct instrument *
_init_gpib(int board, int pad, int sad, spollfun_t sf, unsigned long retry)
{
    struct instrument *new = NULL;
#if HAVE_LINUX_GPIB
    int handle;

    /* dflt: board=0, sad=0, tmo=T30s, eot=1, bin=0, reos=0, xeos=0, eos=0xa */
    handle = ibdev(board, pad, sad, T30s, 1, 0x0a);
    if (handle >= 0) {
        new = _new_inst(GPIB);
        new->d = handle;
        new->sf_fun = sf;
        new->sf_retry = retry;
    }
#else
    fprintf(stderr, "%s: ibdev(%d,%d,0x%x) failed: no GPIB support\n",
            prog, board, pad, sad);
#endif
    return new;
}

static struct instrument *
_init_vxi(const char *name, spollfun_t sf, unsigned long retry)
{
    struct instrument *gd = _new_inst(VXI11);
    int err;

    gd->sf_fun = sf;
    gd->sf_retry = retry;
    gd->vxi11_handle = vxi11_create();

    //vxi11_set_device_debug(true);
    err = vxi11_open(gd->vxi11_handle, (char *)name, false);
    if (err) {
        vxi11_perror(gd->vxi11_handle, err, prog);
        vxi11_destroy(gd->vxi11_handle);
        gd->vxi11_handle = NULL;
        _free_inst(gd);
        gd = NULL;
    }
    return gd;
}

static int
_canon_serial(struct instrument *gd)
{
    struct termios tio;

    if (tcgetattr(gd->fd, &tio) < 0) {
        fprintf(stderr, "%s: error getting serial attributes\n", prog);
        return -1;
    }

    tio.c_lflag |= ICANON;

    tio.c_cc[VEOL] = gd->eos;
    tio.c_cc[VEOF] = 4; /* ctrl-D */

    if (tcsetattr(gd->fd, TCSANOW, &tio) < 0) {
        fprintf(stderr, "%s: error setting serial attributes\n", prog);
        return -1;
    }
    return 0;
}

static int
_raw_serial(struct instrument *gd)
{
    struct termios tio;

    if (tcgetattr(gd->fd, &tio) < 0) {
        fprintf(stderr, "%s: error getting serial attributes\n", prog);
        return -1;
    }

    tio.c_lflag &= ~ICANON;

    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    if (tcsetattr(gd->fd, TCSANOW, &tio) < 0) {
        fprintf(stderr, "%s: error setting serial attributes\n", prog);
        return -1;
    }
    return 0;
}

static struct instrument *
_init_serial(const char *device , char *flags, spollfun_t sf, unsigned long retry)
{
    struct instrument *gd = _new_inst(SERIAL);
    int baud = 9600, databits = 8, stopbits = 1;
    char parity = 'N', flowctl = 'N';
    struct termios tio;
    int i, res;

    gd->fd = open(device, O_RDWR | O_NOCTTY);
    if (gd->fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, device, strerror(errno));
        goto err;
    }
    if (!isatty(gd->fd)) {
        fprintf(stderr, "%s: %s is not a tty\n", prog, device);
        goto err;
    }
    if (lockf(gd->fd, F_TLOCK, 0) < 0) {
        fprintf(stderr, "%s: could not lock %s\n", prog, device);
        goto err;
    }
    (void)sscanf(flags, "%d,%d%c%d,%c", &baud, &databits, &parity, &stopbits,
                 &flowctl);
    /* 0-4 matches OK as defaults are taken if no match */
    if (tcgetattr(gd->fd, &tio) < 0) {
        fprintf(stderr, "%s: error getting serial attributes\n", prog);
        goto err;
    }
    res = -1;
    for (i = 0; i < sizeof(baudmap)/sizeof(baudmap_t); i++) {
        if (baudmap[i].baud == baud) {
            if ((res = cfsetispeed(&tio, baudmap[i].bconst)) == 0)
                 res = cfsetospeed(&tio, baudmap[i].bconst);
            break;
        }
    }
    if (res < 0) {
        fprintf(stderr, "%s: error setting baud rate to %d\n", prog, baud);
        goto err;
    }
    switch (databits) {
        case 7:
            tio.c_cflag &= ~CSIZE;
            tio.c_cflag |= CS7;
            break;
        case 8:
            tio.c_cflag &= ~CSIZE;
            tio.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr, "%s: error setting data bits to %d\n",
                    prog, databits);
            goto err;
    }

    switch (stopbits) {
        case 1:
            tio.c_cflag &= ~CSTOPB;
            break;
        case 2:
            tio.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr, "%s: error setting stop bits to %d\n",
                    prog, stopbits);
            goto err;
    }

    switch (parity) {
        case 'n':
        case 'N':
            tio.c_cflag &= ~PARENB;
            break;
        case 'e':
        case 'E':
            tio.c_cflag |= PARENB;
            tio.c_cflag &= ~PARODD;
            break;
        case 'o':
        case 'O':
            tio.c_cflag |= PARENB;
            tio.c_cflag |= PARODD;
            break;
        default:
            fprintf(stderr, "%s: error setting parity to %c\n", prog, parity);
            goto err;
    }

    switch (flowctl) {
        case 'n':	/* none */
        case 'N':
            tio.c_iflag &= ~(IXON|IXANY|IXOFF);
#ifdef CRTSCTS
            tio.c_cflag &= ~(CRTSCTS);
#else
            tio.c_cflag &= ~(CNEW_RTSCTS);
#endif
            break;
        case 'x':	/* xon/xoff */
        case 'X':
            tio.c_iflag |= (IXON|IXANY|IXOFF);
            break;
        case 'h':	/* hardware handshaking (RTS/CTS) */
        case 'H':
#ifdef CRTSCTS
            tio.c_cflag |= CRTSCTS;
#else
            tio.c_cflag |= CNEW_RTSCTS;
#endif
            break;
        default:
            fprintf(stderr, "%s: unsupported flow control type: %c\n",
                    prog, flowctl);
            goto err;
    }

    tio.c_iflag |= (INPCK | ISTRIP);
    tio.c_cflag |= CLOCAL;
    tio.c_oflag &= ~OPOST;

    /* Set raw mode - _canon_serial() and _raw_serial() may change this
     * when processing eos/reos.
     */
    tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    if (tcsetattr(gd->fd, TCSANOW, &tio) < 0) {
        fprintf(stderr, "%s: error setting serial attributes\n", prog);
        goto err;
    }

    /* success! */
    return gd;

err:
    if (gd->fd >= 0)
        close(gd->fd);
    _free_inst(gd);
    return NULL;
}

static struct instrument *
_init_socket(char *host, char *port, spollfun_t sf, unsigned long retry)
{
    fprintf(stderr, "%s: tcp_init('%s','%s') failed: no socket support\n",
            prog, host, port);
    return NULL;
}

struct instrument *
inst_init(const char *addr, spollfun_t sf, unsigned long retry)
{
    struct instrument *gd = NULL;
    char *endptr, *sfx, *cpy;
    struct stat sb;
    int board, pad, sad;

    cpy = xstrdup(addr);
    if (sscanf(addr, "%d:%d,%d", &board, &pad, &sad) == 3)
        gd = _init_gpib(board, pad, 0x60+sad, sf, retry);/* board:pad,sad */
    if (sscanf(addr, "%d:%d", &board, &pad) == 2)
        gd = _init_gpib(board, pad, 0, sf, retry);       /* board:pad */
    else if (sscanf(addr, "%d,%d", &pad, &sad) == 2)
        gd = _init_gpib(0,pad, 0x60+sad, sf, retry);     /* pad,sad */
    else if ((pad = strtoul(addr, &endptr, 10)) >= 0 && *endptr == '\0')
        gd = _init_gpib(0,     pad, 0,        sf, retry);/* pad */
    else if (stat(addr, &sb) == 0 && S_ISCHR(sb.st_mode))
        gd = _init_serial(addr, "9600,8n1", sf, retry);  /* device */
    else if ((sfx = strchr(cpy, ':'))) {
        *sfx++ = '\0';
        if (stat(cpy, &sb) == 0 && S_ISCHR(sb.st_mode))
            gd = _init_serial(cpy, sfx, sf, retry);  /* device:flags */
        else if (strtoul(sfx, &endptr, 10) > 0 && *endptr == '\0')
            gd = _init_socket(cpy, sfx, sf, retry);  /* host:port */
        else
            gd = _init_vxi(addr, sf, retry);         /* host:inst[,pad[,sad]] */
    } else
        fprintf(stderr, "%s: failed to determine address type\n", prog);
    free(cpy);
    return gd;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
