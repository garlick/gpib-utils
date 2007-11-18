#ifndef HAVE_GPIB
#define HAVE_GPIB 0
#endif

#define _GNU_SOURCE /* for vasprintf */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#if HAVE_GPIB
#include <gpib/ib.h>
#endif
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "gpib.h"
#include "vxi11.h"
#include "util.h"

#define GPIB_DEVICE_MAGIC 0x43435334
struct gpib_device {
    int             magic;
    char           *name;      /* instrument name */
    int             verbose;   /* verbose flag (print telemetry on stderr) */
    int             d;         /* ib instrument handle */
    spollfun_t      sf_fun;    /* app-specific serial poll function */
    int             sf_level;  /* serial poll recursion detection */
    unsigned long   sf_retry;  /* backoff factor for serial poll retry (uS) */
    int             vxi_eos;   /* end of string character */
    int             vxi_reos;  /* flag: terminate receive on eos */
    int             vxi_xeos;  /* flag: automatically append eos */
    int             vxi_eot;   /* flag: assert EOT on last char of write */
    int             vxi_bin;   /* flag: use 8 bits to match eos */
    int             vxi_timeout;/* timeout in milliseconds */
    CLIENT         *vxi_cli;   /* vxi client handle */
    Device_Link     vxi_lid;   /* vxi logical device handle */
    unsigned short  vxi_abort_port; /* vxi abort socket */
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

#if HAVE_GPIB
static int
_ibrd(gd_t gd, void *buf, int len)
{
    ibrd(gd->d, buf, len);
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
#endif

static int
_vxird(gd_t gd, void *buf, int len)
{
    Device_ReadParms p;
    Device_ReadResp *r;
    int res = 0;

    do {
        if (len == 0) {
            fprintf(stderr, "%s: read buffer too small\n", prog);
            gpib_fini(gd);
            exit(1);
        }
        p.lid = gd->vxi_lid;
        p.requestSize = len;
        p.io_timeout = gd->vxi_timeout;
        p.lock_timeout = gd->vxi_timeout;
        p.flags = gd->vxi_reos ? VXI_TERMCHRSET : 0;
        p.termChar = gd->vxi_eos;
        r = device_read_1(&p, gd->vxi_cli);
        if (r == NULL) {
            clnt_perror(gd->vxi_cli, prog);
            gpib_fini(gd);
            exit(1);
        }
        memcpy(buf, r->data.data_val, r->data.data_len);
        res += r->data.data_len;
        if (r->reason == 0) { /* more data available */
            len -= r->data.data_len;
            buf += r->data.data_len;
        }
    } while (len > 0 && r->reason == 0);

    return res;
}

int
gpib_rd(gd_t gd, void *buf, int len)
{
    int count; 

    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        count = _ibrd(gd, buf, len);
    else
#endif
        count = _vxird(gd, buf, len);
    if (gd->verbose)
        fprintf(stderr, "R: [%d bytes]\n", count);
    _serial_poll(gd, "gpib_rd");

    return count;
}

void
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
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
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
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        count = _ibrd(gd, buf, sizeof(buf) - 1);
    else
#endif
        count = _vxird(gd, buf, sizeof(buf) - 1);
    assert(count < sizeof(buf) - 1);
    buf[count] = '\0';
    _zap_trailing_terminators(buf);

    va_start(ap, fmt);
    n = vsscanf(buf, fmt, ap);
    va_end(ap);

    if (gd->verbose) {
        char *cpy = xstrcpyprint(buf);

        fprintf(stderr, "R: \"%s\"\n", cpy);
        free(cpy);
    }
    _serial_poll(gd, "gpib_rdf");

    return n;
}

#if HAVE_GPIB
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
    Device_WriteParms p;
    Device_WriteResp *r;
    char *nbuf = NULL;

    if (gd->vxi_xeos && buf[len - 1] != gd->vxi_eos) {
        nbuf = xmalloc(len + 1);
        memcpy(nbuf, buf, len);
        nbuf[len] = gd->vxi_eos;
    }
    p.lid = gd->vxi_lid;
    p.io_timeout = gd->vxi_timeout;
    p.lock_timeout = gd->vxi_timeout;
    p.flags = gd->vxi_eot ? VXI_ENDW : 0;
    /* XXX: can we handle arbitrarily large buffer here? */
    p.data.data_val = nbuf ? nbuf : buf;
    p.data.data_len = nbuf ? len + 1 : len;
    r = device_write_1(&p, gd->vxi_cli);
    if (nbuf)
        free(nbuf);
    if (r == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
}

void
gpib_wrt(gd_t gd, void *buf, int len)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
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
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
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

int
gpib_wrtf(gd_t gd, char *fmt, ...)
{
    va_list ap;
    char *s;
    int n;

    assert(gd->magic == GPIB_DEVICE_MAGIC);
    va_start(ap, fmt);
    n = vasprintf(&s, fmt, ap);
    va_end(ap);
    if (n == -1) {
        fprintf(stderr, "%s: out of memory\n", prog);
        gpib_fini(gd);
        exit(1);
    }
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
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

    return n;
}


int 
gpib_qry(gd_t gd, char *str, void *buf, int len)
{
    int count; 

    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibwrt(gd, str, strlen(str));
    else
#endif
        _vxiwrt(gd, str, strlen(str));
    if (gd->verbose) {
        char *cpy = xstrcpyprint(str);

        fprintf(stderr, "T: \"%s\"\n", cpy);
        free(cpy);
    }
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        count = _ibrd(gd, buf, len);
    else
#endif
        count = _vxird(gd, buf, len);
    if (gd->verbose) { 
        fprintf(stderr, "R: [%d bytes]\n", count);
    }
    _serial_poll(gd, "gpib_qry");

    return count;
}

#if HAVE_GPIB
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
    Device_GenericParms p;

    p.lid = gd->vxi_lid;
    p.flags = 0;
    p.io_timeout = gd->vxi_timeout;
    p.lock_timeout = gd->vxi_timeout;
    if (device_local_1(&p, gd->vxi_cli) == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
}

void
gpib_loc(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibloc(gd);
    else
#endif
        _vxiloc(gd);
    if (gd->verbose)
        fprintf(stderr, "T: [ibloc]\n");
    _serial_poll(gd, "gpib_loc");
}

#if HAVE_GPIB
static void 
_ibclr(gd_t gd, unsigned long usec)
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
_vxiclr(gd_t gd, unsigned long usec)
{
    Device_GenericParms p;

    p.lid = gd->vxi_lid;
    p.flags = 0;
    p.io_timeout = gd->vxi_timeout;
    p.lock_timeout = gd->vxi_timeout;
    if (device_clear_1(&p, gd->vxi_cli) == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
}

void 
gpib_clr(gd_t gd, unsigned long usec)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibclr(gd, usec);
    else
#endif
        _vxiclr(gd, usec);
    if (gd->verbose)
        fprintf(stderr, "T: [ibclr]\n");
    usleep(usec);
    _serial_poll(gd, "gpib_clr");
}

#if HAVE_GPIB
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
    Device_GenericParms p;

    p.lid = gd->vxi_lid;
    p.flags = 0;
    p.io_timeout = gd->vxi_timeout;
    p.lock_timeout = gd->vxi_timeout;
    if (device_trigger_1(&p, gd->vxi_cli) == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
}

void
gpib_trg(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibtrg(gd);
    else
#endif
        _vxitrg(gd);
    if (gd->verbose)
        fprintf(stderr, "T: [ibtrg]\n");
    _serial_poll(gd, "gpib_trg");
}

#if HAVE_GPIB
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
    Device_GenericParms p;
    Device_ReadStbResp *r;

    p.lid = gd->vxi_lid;
    p.flags = 0;
    p.lock_timeout = gd->vxi_timeout;
    p.io_timeout = gd->vxi_timeout;
    r = device_readstb_1(&p, gd->vxi_cli);
    if (r == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
    if (status)
        *status = r->stb;
    return 0;
}

int
gpib_rsp(gd_t gd, unsigned char *status)
{
    int res;

    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        res = _ibrsp(gd, status);
    else
#endif
        res = _vxirsp(gd, status);
    if (gd->verbose)
        fprintf(stderr, "T: [ibrsp] R: 0x%x\n", (unsigned int)*status);
    return res;
}

#if HAVE_GPIB
static int
_ibgetreos(gd_t gd)
{
    int flag;

    ibask(gd->d, IbaEOSrd, &flag);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibask IbaEOSrd failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }

    return (flag & REOS) ? 1 : 0;
}
#endif

int
gpib_get_reos(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        return _ibgetreos(gd);
#endif
    return gd->vxi_reos;
}

#if HAVE_GPIB
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
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibsetreos(gd, flag);
#endif
    gd->vxi_reos = flag;
}

#if HAVE_GPIB
static int
_ibgetxeos(gd_t gd)
{
    int flag;

    ibask(gd->d, IbaEOSwrt, &flag);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibask IbaEOSwrt failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }

    return (flag & XEOS) ? 1 : 0;
}
#endif

int
gpib_get_xeos(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        return _ibgetxeos(gd);
#endif
    return gd->vxi_xeos;

    return 0;
}

#if HAVE_GPIB
static void
_ibsetxeos(gd_t gd, int flag)
{
    ibconfig(gd->d, IbcEOSwrt, flag ? XEOS : 0);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibconfig IbcEOSwrt failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
}
#endif

void
gpib_set_xeos(gd_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibsetxeos(gd, flag);
#endif
    gd->vxi_xeos = flag;
}

#if HAVE_GPIB
static int
_ibgetbin(gd_t gd)
{
    int flag;

    ibask(gd->d, IbaEOScmp, &flag);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibask IbaEOScmp failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }

    return (flag & BIN) ? 1 : 0;
}
#endif

int
gpib_get_bin(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        return _ibgetbin(gd);
#endif
    return gd->vxi_bin;
    return 0;
}

#if HAVE_GPIB
static void
_ibsetbin(gd_t gd, int flag)
{
    ibconfig(gd->d, IbcEOScmp, flag ? BIN : 0);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibconfig IbcEOScmp failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
}
#endif

void
gpib_set_bin(gd_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibsetbin(gd, flag);
#endif
    gd->vxi_bin = flag;        
}

#if HAVE_GPIB
static int
_ibgeteot(gd_t gd)
{
    int flag;

    ibask(gd->d, IbaEOT, &flag);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibask IbaEOT failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }

    return flag;
}
#endif

int
gpib_get_eot(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        return _ibgeteot(gd);
#endif
    return gd->vxi_eot;
}

#if HAVE_GPIB
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
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibseteot(gd, flag);
#endif
    gd->vxi_eot = flag;
}

#if HAVE_GPIB
int
_ibgeteos(gd_t gd)
{
    int c;

    ibask(gd->d, IbaEOSchar, &c);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibask IbaEOSchar failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }

    return c;
}
#endif

int
gpib_get_eos(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        return _ibgeteos(gd);
#endif
    return gd->vxi_eos;
}

#if HAVE_GPIB
void
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
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibseteos(gd, c);
#endif
    gd->vxi_eos = c;
}

#if HAVE_GPIB
double
_ibgettmo(gd_t gd)
{
    int result;
    double sec = 0;

    ibask(gd->d, IbaTMO, &result);
    if ((ibsta & ERR)) {
        fprintf(stderr, "%s: ibask failed: %d\n", prog, iberr);
        gpib_fini(gd);
        exit(1);
    }
    switch (result) {
        case TNONE:     sec = 0; break;
        case T10us:     sec = 0.00001; break;
        case T30us:     sec = 0.00003; break;
        case T100us:    sec = 0.00010; break;
        case T300us:    sec = 0.00030; break;
        case T1ms:      sec = 0.00100; break;
        case T3ms:      sec = 0.00300; break;
        case T10ms:     sec = 0.01000; break;
        case T30ms:     sec = 0.03000; break;
        case T100ms:    sec = 0.10000; break;
        case T300ms:    sec = 0.30000; break;
        case T1s:       sec = 1.00000; break;
        case T3s:       sec = 3.00000; break;
        case T10s:      sec = 10.0000; break;
        case T30s:      sec = 30.0000; break;
        case T100s:     sec = 100.000; break;
        case T300s:     sec = 300.000; break;
        case T1000s:    sec = 1000.00; break;
    }
    return sec;
}
#endif

double
gpib_get_timeout(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        return _ibgettmo(gd);
#endif
    return 1000.0*gd->vxi_timeout;
}

#if HAVE_GPIB
static void
_ibtmo(gd_t gd, double sec)
{
    int val;
    char *str;

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
#if HAVE_GPIB
    if (gd->vxi_cli == NULL)
        _ibtmo(gd, sec);
#endif
    gd->vxi_timeout = sec / 1000.0;
}

void
gpib_set_verbose(gd_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    gd->verbose = flag;
}

int
gpib_get_verbose(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);

    return gd->verbose;
}

void
_free_gpib(gd_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    /*assert(gd->sf_level == 0);*/
    gd->magic = 0;
    free(gd);
}

void
gpib_fini(gd_t gd)
{
    if (gd->vxi_lid != -1)
        destroy_link_1(&gd->vxi_lid, gd->vxi_cli);
    if (gd->vxi_cli)
        clnt_destroy(gd->vxi_cli);
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
    new->sf_retry = 0;
    new->vxi_cli = NULL;
    new->vxi_lid = -1;
    new->vxi_abort_port = -1;
    new->vxi_timeout = 30000; /* 30s */
    new->vxi_eot = 1;
    new->vxi_bin = 0;
    new->vxi_reos = 0;
    new->vxi_xeos = 0;
    new->vxi_eos = 0xa;

    return new;
}

#if HAVE_GPIB
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
_init_vxi(char *host, char *device, spollfun_t sf, unsigned long retry)
{
    gd_t new = _new_gpib();
    Create_LinkParms p;
    Create_LinkResp *r;

    new->sf_fun = sf;
    new->sf_retry = retry;
    new->vxi_cli = clnt_create(host, DEVICE_CORE, DEVICE_CORE_VERSION, "tcp");
    if (new->vxi_cli == NULL) {
        clnt_pcreateerror(prog);
        exit(1);
    }
    p.clientId = 0;
    p.lockDevice = 0;
    p.lock_timeout = 10000; /* not used */
    p.device = device;
    r = create_link_1(&p, new->vxi_cli);
    if (r == NULL) {
        clnt_perror(new->vxi_cli, prog);
        gpib_fini(new);
        exit(1);
    }
    new->vxi_lid = r->lid;
    new->vxi_abort_port = r->abortPort;
      
    return new;
}

gd_t
gpib_init(char *addr, spollfun_t sf, unsigned long retry)
{
    gd_t res = NULL;

    if (strchr(addr, ':') != NULL) {
        char *cpy = xstrdup(addr);
        char *p = strchr(cpy, ':');

        *p++ = '\0';
        res = _init_vxi(cpy, p, sf, retry);
        free(cpy);
    } else { 
#if HAVE_GPIB
        res = _ibdev(strtoul(addr, NULL, 10), sf, retry);
#endif
    }
    return res;
}

static char *
_parse_line(char *buf, char *key)
{
    char *k, *v;

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
