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
#define __USE_ISOC99 /* activate vsscanf prototype in stdio.h */
#include <stdio.h>

#include "gpib.h"
#include "vxi11_device.h"
#include "util.h"
#include "hprintf.h"

#define GPIB_DEVICE_MAGIC 0x43435334
struct gpib_device {
    int             magic;
    char           *name;      /* instrument name */
    int             verbose;   /* verbose flag (print telemetry on stderr) */
    int             d;         /* ib instrument handle */
    spollfun_t      sf_fun;    /* app-specific serial poll function */
    int             sf_level;  /* serial poll recursion detection */
    unsigned long   sf_retry;  /* backoff factor for serial poll retry (uS) */
    vxi11dev_t      vxi11_handle;
};

extern char *prog;

/* If a serial poll function is defined, call it with the instrument
 * status byte.
 */
static void
_serial_poll(gd_t gd, char *str)
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
            more = gpib_rsp(gd, &sb);
            err = gd->sf_fun(gd, sb, str);
            switch (err) {
                case -1:    /* retry - device not ready (and we care) */
                    usleep(++loops*gd->sf_retry);
                    break;
                case 0:     /* no error */
                    break;
                default:    /* fatal error */
                    gpib_fini(gd);
                    exit(err);
            }
        } while (more || err == -1);
    }
    gd->sf_level--;
}

#if HAVE_LINUX_GPIB
static int
_ibrd(gd_t gd, char *buf, int len)
{
    int count = 0;

    do {
        ibrd(gd->d, buf + count, len - count);
        if (ibsta & TIMO) {
            fprintf(stderr, "%s: ibrd timeout\n", prog);
            gpib_fini(gd);
            exit(1);
        }
        if (ibsta & ERR) {
            fprintf(stderr, "%s: ibrd error %d\n", prog, iberr);
            gpib_fini(gd);
            exit(1);
        }
        count += ibcnt;
    } while (count < len && !(ibsta & END));
    if (!(ibsta & END)) {
        fprintf(stderr, "%s: read buffer too small\n", prog);
        gpib_fini(gd);
        exit(1);
    }
    return count;
}
#endif

static int
_vxird(gd_t gd, char *buf, int len)
{
    int err, count;

    err = vxi11_read(gd->vxi11_handle, buf, len, &count);
    if (err) {
        vxi11_perror(gd->vxi11_handle, err, prog);
        gpib_fini(gd);
        exit(1);
    }
    return count;
}

int
gpib_rd(gd_t gd, void *buf, int len)
{
    int count; 

    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        count = _ibrd(gd, buf, len);
    else
#endif
    count = _vxird(gd, buf, len);
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
gpib_rdstr(gd_t gd, char *buf, int len)
{
    int count;

    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        count = _ibrd(gd, buf, len - 1);
    else
#endif
    count = _vxird(gd, buf, len - 1);
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
gpib_rdf(gd_t gd, char *fmt, ...)
{
    va_list ap;
    char buf[1024];
    int count;
    int n;

    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        count = _ibrd(gd, buf, sizeof(buf) - 1);
    else
#endif
    count = _vxird(gd, buf, sizeof(buf) - 1);
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

#if HAVE_LINUX_GPIB
static void 
_ibwrt(gd_t gd, void *buf, int len)
{
    ibwrt(gd->d, buf, len);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibwrt timeout\n", prog);
        gpib_fini(gd);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibwrt error %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
    assert(ibcnt == len);
}
#endif

static void 
_vxiwrt(gd_t gd, char *buf, int len)
{
    int err;

    err = vxi11_write(gd->vxi11_handle, buf, len);
    if (err) {
        vxi11_perror(gd->vxi11_handle, err, prog);
        gpib_fini(gd);
        exit(1);
    }
}

void
gpib_wrt(gd_t gd, void *buf, int len)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibwrt(gd, buf, len);
    else
#endif
    _vxiwrt(gd, buf, len);
    if (gd->verbose)
        fprintf(stderr, "T: [%d bytes]\n", len);
    _serial_poll(gd, "gpib_wrt");
}

void 
gpib_wrtstr(gd_t gd, char *str)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibwrt(gd, str, strlen(str));
    else
#endif
    _vxiwrt(gd, str, strlen(str));
    if (gd->verbose) {
        char *cpy = xstrcpyprint(str);

        fprintf(stderr, "T: \"%s\"\n", cpy);
        free(cpy);
    }
    _serial_poll(gd, "gpib_wrtstr");
}

void
gpib_wrtf(gd_t gd, char *fmt, ...)
{
    va_list ap;
    char *s;

    assert(gd->magic == GPIB_DEVICE_MAGIC);
    va_start(ap, fmt);
    s = hvsprintf(fmt, ap);
    va_end(ap);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibwrt(gd, s, strlen(s));
    else
#endif
    _vxiwrt(gd, s, strlen(s));
    if (gd->verbose) {
        char *cpy = xstrcpyprint(s);

        fprintf(stderr, "T: \"%s\"\n", cpy);
        free(cpy);
    }
    free(s);
    _serial_poll(gd, "gpib_wrtf");
}

int 
gpib_qry(gd_t gd, char *str, void *buf, int len)
{
    int count; 

    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibwrt(gd, str, strlen(str));
    else
#endif
    _vxiwrt(gd, str, strlen(str));
    if (gd->verbose) {
        char *cpy = xstrcpyprint(str);

        fprintf(stderr, "T: \"%s\"\n", cpy);
        free(cpy);
    }
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        count = _ibrd(gd, buf, len);
    else
#endif
    count = _vxird(gd, buf, len);
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

#if HAVE_LINUX_GPIB
static void
_ibloc(gd_t gd)
{
    ibloc(gd->d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibloc error %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
}
#endif

static void
_vxiloc(gd_t gd)
{
    int err;

    err = vxi11_local(gd->vxi11_handle);
    if (err) {
        vxi11_perror(gd->vxi11_handle, err, prog);
        exit(1);
    }
}

void
gpib_loc(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibloc(gd);
    else
#endif
    _vxiloc(gd);
    if (gd->verbose)
        fprintf(stderr, "T: [ibloc]\n");
    _serial_poll(gd, "gpib_loc");
}

#if HAVE_LINUX_GPIB
static void 
_ibclr(gd_t gd)
{
    ibclr(gd->d);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibclr timeout\n", prog);
        gpib_fini(gd);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibclr error %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
}
#endif

static void 
_vxiclr(gd_t gd)
{
    int err;

    err = vxi11_clear(gd->vxi11_handle);
    if (err) {
        vxi11_perror(gd->vxi11_handle, err, prog);
        exit(1);
    }
}

void 
gpib_clr(gd_t gd, unsigned long usec)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibclr(gd);
    else
#endif
    _vxiclr(gd);
    if (gd->verbose)
        fprintf(stderr, "T: [ibclr]\n");
    usleep(usec);
    _serial_poll(gd, "gpib_clr");
}

#if HAVE_LINUX_GPIB
static void
_ibtrg(gd_t gd)
{
    ibtrg(gd->d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibtrg error %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
}
#endif

static void
_vxitrg(gd_t gd)
{
    int err;

    err = vxi11_trigger(gd->vxi11_handle);
    if (err) {
        vxi11_perror(gd->vxi11_handle, err, prog);
        exit(1);
    }
}

void
gpib_trg(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibtrg(gd);
    else
#endif
    _vxitrg(gd);
    if (gd->verbose)
        fprintf(stderr, "T: [ibtrg]\n");
    _serial_poll(gd, "gpib_trg");
}

#if HAVE_LINUX_GPIB
static int
_ibrsp(gd_t gd, unsigned char *status)
{
    ibrsp(gd->d, (char *)status); /* NOTE: modifies ibcnt */
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrsp error %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
    /* nonzero means call gpib_rsp again to obtain more status info */
    return (ibsta & RQS);
}
#endif

static int
_vxirsp(gd_t gd, unsigned char *status)
{
    int err;

    err = vxi11_readstb(gd->vxi11_handle, status);
    if (err) {
        vxi11_perror(gd->vxi11_handle, err, prog);
        exit(1);
    }
    return 0;
}

int
gpib_rsp(gd_t gd, unsigned char *status)
{
    int res;

    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        res = _ibrsp(gd, status);
    else
#endif
    res = _vxirsp(gd, status);
    if (gd->verbose)
        fprintf(stderr, "T: [ibrsp] R: 0x%x\n", (unsigned int)*status);
    return res;
}

#if HAVE_LINUX_GPIB
static void
_ibsetreos(gd_t gd, int flag)
{
    ibconfig(gd->d, IbcEOSrd, flag ? REOS : 0);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibconfig IbcEOSrd failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
}
#endif

void
gpib_set_reos(gd_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibsetreos(gd, flag);
    else
#endif
    vxi11_set_termcharset(gd->vxi11_handle, flag);
}

#if HAVE_LINUX_GPIB
static void
_ibseteot(gd_t gd, int flag)
{
    ibconfig(gd->d, IbcEOT, flag);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibconfig IbcEOT failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
}
#endif

void
gpib_set_eot(gd_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibseteot(gd, flag);
    else
#endif
    vxi11_set_endw(gd->vxi11_handle, flag);
}

#if HAVE_LINUX_GPIB
static void
_ibseteos(gd_t gd, int c)
{
    ibconfig(gd->d, IbcEOSchar, c);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibconfig IbcEOSchar failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
}
#endif

void
gpib_set_eos(gd_t gd, int c)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibseteos(gd, c);
    else
#endif
    vxi11_set_termchar(gd->vxi11_handle, c);
}

#if HAVE_LINUX_GPIB
static void
_ibtmo(gd_t gd, double sec)
{
    char *str;
    int val = T1000s;

    if (sec < 0 || sec > 1000) {
        fprintf(stderr, "%s: gpib_set_timeout: timeout out of range\n", prog);
        gpib_fini(gd);
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
        gpib_fini(gd);
        exit(1);
    }
}
#endif

void
gpib_set_timeout(gd_t gd, double sec)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_LINUX_GPIB
    if (gd->vxi11_handle == NULL)
        _ibtmo(gd, sec);
    else
#endif
        vxi11_set_iotimeout(gd->vxi11_handle, sec * 1000.0);
}

void
gpib_set_verbose(gd_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    gd->verbose = flag;
}

static void
_free_gpib(gd_t gd)
{
    gd->magic = 0;
    free(gd);
}

/* Call to abort in-progress RPC on core channel.
 */
void
gpib_abort(gd_t gd)
{
    int err;

    if (gd->vxi11_handle) {
        err = vxi11_abort(gd->vxi11_handle);
        if (err) /* N.B. non-fatal */
            vxi11_perror(gd->vxi11_handle, err, prog);
    }
}

void
gpib_fini(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    if (gd->vxi11_handle) {
        vxi11_close(gd->vxi11_handle);
        vxi11_destroy(gd->vxi11_handle);
    }
    _free_gpib(gd);
}

static gd_t
_new_gpib(void)
{
    gd_t new = xmalloc(sizeof(struct gpib_device));

    new->magic = GPIB_DEVICE_MAGIC;
    new->d = -1;
    new->verbose = 0;
    new->sf_fun = NULL;
    new->sf_level = 0;
    new->sf_retry = 1;
    new->vxi11_handle = NULL;

    return new;
}

#if HAVE_LINUX_GPIB
static gd_t
_ibdev(int pad, spollfun_t sf, unsigned long retry)
{
    int handle;
    gd_t new = NULL;

    /* dflt: board=0, sad=0, tmo=T30s, eot=1, bin=0, reos=0, xeos=0, eos=0xa */
    handle = ibdev(0, pad, 0, T30s, 1, 0x0a);
    if (handle >= 0) {
        new = _new_gpib();
        new->d = handle;
        new->sf_fun = sf;
        new->sf_retry = retry;
    }
    return new;
}
#endif

static gd_t
_init_vxi(char *name, spollfun_t sf, unsigned long retry)
{
    gd_t gd = _new_gpib();
    int err;

    gd->sf_fun = sf;
    gd->sf_retry = retry;
    gd->vxi11_handle = vxi11_create();

    //vxi11_set_device_debug(1);
    err = vxi11_open(gd->vxi11_handle, name, 0);
    if (err) {
        vxi11_perror(gd->vxi11_handle, err, prog);
        exit(1);
    }
    return gd;
}

gd_t
gpib_init(char *addr, spollfun_t sf, unsigned long retry)
{
    gd_t gd = NULL;

    if (strchr(addr, ':') == NULL) { 
#if HAVE_LINUX_GPIB
        gd = _ibdev(strtoul(addr, NULL, 10), sf, retry);
#else
        fprintf(stderr, "%s: not configured with native GPIB support\n", prog);
#endif
    } else
        gd = _init_vxi(addr, sf, retry);
    return gd;
}

static char *
_parse_line(char *buf, char *key)
{
    char *k;

    for (k = buf; *k != '\0'; k++)
        if (*k == '#')
            *k = '\0';
    k = strtok(buf, " \t\r\n");
    if (!k || strcmp(k, key) != 0)
        return NULL;
    return strtok(NULL, " \t\r\n");
}

char *
gpib_default_addr(char *name)
{
    static char buf[64];
    FILE *cf;
    char *res = NULL;

    if (!(cf = fopen("/etc/gpib-utils.conf", "r")))
        cf = fopen("../etc/gpib-utils.conf", "r");
    if (cf) {
        while (res == NULL && fgets(buf, sizeof(buf), cf) != NULL)
            res = _parse_line(buf, name);
        (void)fclose(cf);
    }
    return res;
}

static int
_extract_dlab_len(unsigned char *data, int lenlen, int len)
{
    char tmpstr[64];
  
    if (lenlen > len || lenlen > sizeof(tmpstr) - 1)
       return -1;
    memcpy(tmpstr, data, lenlen);
    tmpstr[lenlen] = '\0';
    return strtoul(tmpstr, NULL, 10);
}

/* Verify/decode some 488.2 data formats.
 */
int
gpib_decode_488_2_data(unsigned char *data, int *lenp, int flags)
{
    int len = *lenp;

    if (len < 3)
        return -1;

    /* 8.7.10 indefinite length arbitrary block response data */
    if (data[0] == '#' && data[1] == '0' && data[len - 1] == '\n') {
        if (!(flags & GPIB_DECODE_ILAB) && !(flags & GPIB_VERIFY_ILAB)) {
            fprintf(stderr, "%s: unexpectedly got ILAB response\n", prog);
            return -1;
        }
        if (flags & GPIB_DECODE_ILAB) {
            memmove(data, &data[2], len - 3);
            *lenp -= 3;
        }

    /* 8.7.9 definite length arbitrary block response data */
    } else if (data[0] == '#' && data[1] >= '1' && data[1] <= '9') {
        int llen = data[1] - '0';
        int dlen = _extract_dlab_len(&data[2], llen, len - 2);

        if (!(flags & GPIB_DECODE_DLAB) && !(flags & GPIB_VERIFY_DLAB)) {
            fprintf(stderr, "%s: unexpectedly got DLAB response\n", prog);
            return -1;
        }
        dlen = _extract_dlab_len(&data[2], data[1] - '0', len - 2);
        if (dlen == -1) {
            fprintf(stderr, "%s: failed to decode DLAB data length\n", prog);
            return -1;
        }
        if (dlen + 2 + llen > len) {
            fprintf(stderr, "%s: DLAB data length is too large\n", prog);
            return -1;
        }
        if (flags & GPIB_DECODE_DLAB) {
            memmove(data, data + 2 + llen, dlen);
            *lenp = dlen;
        }
    } else {
        fprintf(stderr, "%s : unexpected or garbled 488.2 response\n", prog);
        return -1;
    }

    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
