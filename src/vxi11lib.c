#define _GNU_SOURCE /* for vasprintf */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <rpc/pmap_clnt.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "gpib.h"
#include "vxi11.h"
#include "vxi11intr.h"
#include "util.h"

#define VXI_HANDLE_MAGIC  0x4c4ba309
struct vxi_handle {
    int             magic;
    CLIENT         *vxi_cli;    /* core channel */
    CLIENT         *vxi_abrt;   /* abort channel */
    Device_Link     vxi_lid;    /* device link */
    vxi_error_t     vxi_error;  /* result of last operation */
};

int
vxi11_device_read(vxi_handle_t vp, char *buf, int len, 
                  int flags, int locktmout, int iotmout, int termchar)
{
    Device_ReadParms p;
    Device_ReadResp *r;

    p.lid = vp->vxi_lid;
    p.requestSize = len;
    p.io_timeout = iotmout;
    p.lock_timeout = locktmout;
    p.flags = flags;
    p.termChar = termchar;
    r = device_read_1(&p, vp->vxi_cli);
    if (r == NULL) {
        clnt_perror(vp->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
    vp->
        if (r->error != 0) {
            fprintf(stderr, "%s: read returned error: %ld\n", prog, r->error);
        }
        memcpy(buf + count, r->data.data_val, r->data.data_len);
        count += r->data.data_len;
    } while (count < len && r->reason == 0);
    if (r->reason == 0) {
        fprintf(stderr, "%s: read buffer too small\n", prog);
        gpib_fini(gd);
        exit(1);
    }
    return count;
}

void 
vxi11_device_write(vxi_handle_t gd, char *buf, int len)
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
    if (r->error != 0) {
        fprintf(stderr, "%s: write returned error: %ld\n", prog, r->error);
        exit(1);
    }
}

void
vxi11_device_local(vxi_handle_t gd)
{
    Device_GenericParms p;
    Device_Error *r;

    p.lid = gd->vxi_lid;
    p.flags = 0;
    p.io_timeout = gd->vxi_timeout;
    p.lock_timeout = gd->vxi_timeout;
    if ((r = device_local_1(&p, gd->vxi_cli)) == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
    if (r->error)
        fprintf(stderr, "%s: device_local: error %ld\n", prog, r->error);
}

void 
vxi11_device_clear(vxi_handle_t gd, unsigned long usec)
{
    Device_GenericParms p;
    Device_Error *r;

    p.lid = gd->vxi_lid;
    p.flags = 0;
    p.io_timeout = gd->vxi_timeout;
    p.lock_timeout = gd->vxi_timeout;
    if ((r = device_clear_1(&p, gd->vxi_cli)) == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
    if (r->error)
        fprintf(stderr, "%s: device_clear: error %ld\n", prog, r->error);
}

void
vxi11_device_trigger(vxi_handle_t gd)
{
    Device_GenericParms p;
    Device_Error *r;

    p.lid = gd->vxi_lid;
    p.flags = 0;
    p.io_timeout = gd->vxi_timeout;
    p.lock_timeout = gd->vxi_timeout;
    if ((r = device_trigger_1(&p, gd->vxi_cli)) == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
    if (r->error)
        fprintf(stderr, "%s: device_trigger: error %ld\n", prog, r->error);
}

int
vxi11_device_readstb(vxi_handle_t gd, unsigned char *status)
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
    if (r->error)
        fprintf(stderr, "%s: device_readstb: error %ld\n", prog, r->error);
    if (status)
        *status = r->stb;
    return 0;
}

void
vxi11_device_abort(vxi_handle_t gd)
{
    Device_Error *r;

    if (gd->vxi_cli != NULL) {
        if ((r = device_abort_1(&gd->vxi_lid, gd->vxi_abrt)) == NULL)
            clnt_perror(gd->vxi_abrt, prog); 
        else if (r->error)
            fprintf(stderr, "%s: device_abort: error %ld\n", prog, r->error);
    }
}

void
vxi11_device_docmd(vxi_handle_t gd, vxi11_docmd_cmd_t cmd, ...)
{
    va_list ap;
    Device_DocmdParms p;
    Device_DocmdResp *r;

    assert(gd->magic == GPIB_DEVICE_MAGIC);

    /* va_start(ap, cmd) */
    /*   setup differently for each cmd */
    /* va_end(ap) */

    p.lid = gd->vxi_lid;
    p.flags = 0;
    p.io_timeout = gd->vxi_timeout;
    p.lock_timeout = gd->vxi_timeout;
    p.cmd = cmd;
    p.data_in.data_in_len = 0;
    r = device_docmd_1(&p, gd->vxi_cli);
    if (r == NULL) {
        clnt_perror(gd->vxi_cli, prog);
        gpib_fini(gd);
        exit(1);
    }
    if (r->error) {
         fprintf(stderr, "%s: device_docmd: error %ld\n", prog, r->error);
         exit(1);
    }
}


int
vxi11_get_reos(vxi_handle_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    return gd->vxi_reos;
}

void
vxi11_set_reos(vxi_handle_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    gd->vxi_reos = flag;
}

int
vxi11_get_xeos(vxi_handle_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    return gd->vxi_xeos;
}

void
vxi11_set_xeos(vxi_handle_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    gd->vxi_xeos = flag;
}

int
vxi11_get_bin(vxi_handle_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    return gd->vxi_bin;
}

void
vxi11_set_bin(vxi_handle_t gd, int flag)
{
    gd->vxi_bin = flag;        
}

int
vxi11_get_eot(vxi_handle_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    return gd->vxi_eot;
}

void
vxi11_set_eot(vxi_handle_t gd, int flag)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    gd->vxi_eot = flag;
}

int
vxi11_get_eos(vxi_handle_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    return gd->vxi_eos;
}

void
vxi11_set_eos(vxi_handle_t gd, int c)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    gd->vxi_eos = c;
}

double
vxi11_get_timeout(vxi_handle_t gd)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    return gd->vxi_timeout;
}

void
vxi11_set_timeout(vxi_handle_t gd, int sec)
{
    assert(gd->magic == GPIB_DEVICE_MAGIC);
    gd->vxi_timeout = ms;
}

static void
_free_gpib(vxi_handle_t gd)
{
    gd->magic = 0;
    free(gd);
}

void
gpib_fini(vxi_handle_t gd)
{
    Device_Error *r;

    assert(gd->magic == GPIB_DEVICE_MAGIC);
    if (gd->vxi_lid != -1) {
        if ((r = destroy_link_1(&gd->vxi_lid, gd->vxi_cli)) == NULL)
            clnt_perror(gd->vxi_cli, prog);
        else if (r->error)
            fprintf(stderr, "%s: destroy_link: error %ld\n", prog, r->error);
    }
    if (gd->vxi_abrt)
        clnt_destroy(gd->vxi_abrt);
    if (gd->vxi_cli)
        clnt_destroy(gd->vxi_cli);
    _free_gpib(gd);
}

static vxi_handle_t
_new_gpib(void)
{
    vxi_handle_t new = xmalloc(sizeof(struct gpib_device));

    new->magic = GPIB_DEVICE_MAGIC;
    new->d = -1;
    new->verbose = 0;
    new->sf_fun = NULL;
    new->sf_level = 0;
    new->sf_retry = 1;
    new->vxi_cli = NULL;
    new->vxi_abrt = NULL;
    new->vxi_lid = -1;
    new->vxi_timeout = 30000; /* 30s */
    new->vxi_eot = 1;
    new->vxi_bin = 0;
    new->vxi_reos = 0;
    new->vxi_xeos = 0;
    new->vxi_eos = 0xa;
    new->vxi_srqthread;

    return new;
}

#if HAVE_GPIB
static vxi_handle_t
_ibdev(int pad, spollfun_t sf, unsigned long retry)
{
    int handle;
    vxi_handle_t new = NULL;

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

/* SRQ service function */
void *
device_intr_srq_1_svc(Device_SrqParms *p, struct svc_req *rq)
{
    char *tmpstr = xmalloc(p->handle.handle_len + 1);

    memcpy(tmpstr, p->handle.handle_val, p->handle.handle_len);
    tmpstr[p->handle.handle_len] = '\0';
    fprintf(stderr, "%s: SRQ received for handle '%s'\n", prog, tmpstr);
            
    return NULL;
}

extern void device_intr_1(struct svc_req *, register SVCXPRT *);

static void *
_vxisrq_thread(void *arg)
{
    SVCXPRT *transp;
    
    pmap_unset(DEVICE_INTR, DEVICE_INTR_VERSION);
    
    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL) {
        fprintf(stderr, "%s: cannot create udp SRQ service\n", prog);
        return NULL;
    }
    if (!svc_register(transp, DEVICE_INTR, DEVICE_INTR_VERSION, 
                device_intr_1, IPPROTO_UDP)) {
        fprintf(stderr, "%s: unable to register udp SRQ service\n", prog);
        return NULL;
    }
    
    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        fprintf(stderr, "%s: cannot create tcp SRQ service\n", prog);
        return NULL;
    }
    if (!svc_register(transp, DEVICE_INTR, DEVICE_INTR_VERSION, 
                device_intr_1, IPPROTO_TCP)) {
        fprintf(stderr, "%s: unable to register tcp SRQ service\n", prog);
        return NULL;
    }
    svc_run ();
    fprintf(stderr, "%s: error: SRQ svc_run returned\n", prog);
    return NULL;
}


static int
_get_sockaddr(char *host, int p, struct sockaddr_in *sap)
{
    struct addrinfo *aip;
    char port[16];
    int err;

    snprintf(port, sizeof(port), "%d", p);
    if ((err = getaddrinfo(host, port, NULL, &aip))) {
        fprintf(stderr, "%s: getaddrinfo: %s\n", prog, gai_strerror(err));
        return -1;
    }
    assert(sizeof (struct sockaddr_in) == sizeof(struct sockaddr));
    memcpy(sap, aip->ai_addr, sizeof(struct sockaddr));
    freeaddrinfo(aip);
    return 0;
}

static vxi_handle_t
_init_vxi(char *host, char *device, spollfun_t sf, unsigned long retry)
{
    vxi_handle_t new = _new_gpib();
    Create_LinkParms p;
    Create_LinkResp *r;
    struct sockaddr_in addr;
    int sock;

    new->sf_fun = sf;
    new->sf_retry = retry;

    /* open core connection */
    new->vxi_cli = clnt_create(host, DEVICE_CORE, DEVICE_CORE_VERSION, "tcp");
    if (new->vxi_cli == NULL) {
        clnt_pcreateerror(prog);
        exit(1);
    }

    /* establish link to instrument */
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

    /* open async connection to instrument for RPC abort */
    if (_get_sockaddr(host, r->abortPort, &addr) == -1) {
        gpib_fini(new);
        exit(1);
    }
    sock = RPC_ANYSOCK;
    new->vxi_abrt = clnttcp_create(&addr, DEVICE_ASYNC, DEVICE_ASYNC_VERSION, 
            &sock, 0, r->maxRecvSize);
    if (new->vxi_abrt == NULL) {
        clnt_pcreateerror(prog);
        gpib_fini(new);
        exit(1);
    }

#if 0
    /* start srq service thread */
    if (pthread_create(&new->vxi_srqthread, NULL, _vxisrq_thread, NULL) != 0)
        fprintf(stderr, "%s: pthread_create _vxisrq_thread failed\n", prog);
#endif
    return new;
}

vxi_handle_t
gpib_init(char *addr, spollfun_t sf, unsigned long retry)
{
    vxi_handle_t res = NULL;

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
