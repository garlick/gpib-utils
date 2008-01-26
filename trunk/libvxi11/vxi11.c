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
#include <stdint.h>

#include "vxi11.h"
#include "vxi11_private.h"
#include "vxi11intr.h"

#define VXI11_DEFAULT_EOS         0x0c
#define VXI11_DEFAULT_REOS        1
#define VXI11_DEFAULT_EOT         1
#define VXI11_DEFAULT_LOCKTIMEOUT 30000 /* 30s */
#define VXI11_DEFAULT_IOTIMEOUT   30000 /* 30s */

#define VXI11_CHANNEL_MAGIC  0xa92cbacd
struct vxi11_channel_struct {
    int             chan_magic;
    CLIENT         *chan_core;   /* core channel */
    vxi11_device_t  chan_devices;
};

#define VXI11_DEVICE_MAGIC  0x4c4ba309
struct vxi11_device_struct {
    int             dev_magic;
    vxi11_channel_t dev_channel;    /* core channel struct */
    CLIENT         *dev_abrt;       /* abort channel */
    SVCXPRT        *dev_intr_svc;   /* intr service */
    srq_fun_t       dev_intr_fun;   /* intr callback */
    void           *dev_intr_arg;   /* opaque (to us) arg for callback */
    Device_Link     dev_lid;        /* device link */
    unsigned long   dev_maxrecvsize;/* max write device will accept */
    unsigned char   dev_eos;        /* termchar used in read */
    int             dev_reos;       /* if true, termchar can terminate read */
    int             dev_eot;        /* if true, assert EOI on write */
    int             dev_waitlock;   /* if true, block if device locked */
    unsigned long   dev_locktimeout;/* lock timeout in milliseconds */
    unsigned long   dev_iotimeout;  /* io timeout in milliseconds */
    vxi11_device_t  dev_prev;
    vxi11_device_t  dev_next;
};

typedef struct {
    int     num;
    char   *str;
} strtab_t;
static strtab_t vxi11_errtab[] = VXI11_ERRORS;

#define RETURN_ERR(ep, e, r) { if (ep) *(ep) = e; return (r); }
#define RETURN_OK(ep, r)     { if (ep) *(ep) = VXI11_ERR_SUCCESS; return (r); }

#ifndef MIN
#define MIN(x,y)    ((x) < (y) ? (x) : (y))
#endif

/* device accessor for core channel */
static inline
CLIENT *
_chan_core(vxi11_device_t vp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    assert(vp->dev_channel != NULL);
    assert(vp->dev_channel->chan_magic == VXI11_CHANNEL_MAGIC);

    return vp->dev_channel->chan_core;
}

char *
vxi11_strerror(long err)
{
    strtab_t *tp;
    char *res = "unknown error";

    if ((err & VXI11_ERR_RPCERR)) {
        res = clnt_sperrno((enum clnt_stat)(err & ~VXI11_ERR_RPCERR));
    } else {
        for (tp = &vxi11_errtab[0]; tp->str; tp++) {
            if (tp->num == err) {
                res = tp->str;
                break;
            }
        }
    }
    return res;
}

static long
_rpcerr(CLIENT *cli)
{
    struct rpc_err rpcerr;
    long err;

    /* see rpc/clnt.h */

    if (cli) {
        clnt_geterr(cli, &rpcerr);
        assert(rpcerr.re_status < VXI11_ERR_RPCERR);
        err = rpcerr.re_status | VXI11_ERR_RPCERR;
    } else {
        assert(rpc_createerr.cf_stat < VXI11_ERR_RPCERR);
        err = rpc_createerr.cf_stat | VXI11_ERR_RPCERR;
    }

    return err;
}

int
vxi11_device_read(vxi11_device_t vp, char *buf, int len, 
                  int *reasonp, long *errp)
{
    Device_ReadParms p;
    Device_ReadResp *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    p.lid                 = vp->dev_lid;
    p.requestSize         = len;
    p.io_timeout          = vp->dev_iotimeout;
    p.lock_timeout        = vp->dev_locktimeout;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK   : 0;
    p.flags              |= vp->dev_reos     ? VXI11_FLAG_TERMCHRSET : 0;
    p.termChar            = vp->dev_eos;
    r = device_read_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error != 0)
        RETURN_ERR(errp, r->error, -1);
    if (r->data.data_len > len)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (r->data.data_len == len && !(r->reason & VXI11_REASON_REQCNT))
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (r->data.data_len < len && (r->reason & VXI11_REASON_REQCNT))
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (!vp->dev_reos && (r->reason & VXI11_REASON_CHR))
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    memcpy(buf, r->data.data_val, r->data.data_len);
    if (reasonp)
        *reasonp = r->reason;
    RETURN_OK(errp, r->data.data_len);
    /* observation B.6.9: short read possible (reason == 0 and error == 0) */
}

int
vxi11_device_readall(vxi11_device_t vp, char *buf, int len, long *errp)
{
    int n, count = 0;
    int reason;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    do {
        n = vxi11_device_read(vp, buf + count, len - count, &reason, errp);
        if (n == -1)
            return -1;
        count += n;
    } while (reason == 0);
    RETURN_OK(errp, count);
}

int
vxi11_device_write(vxi11_device_t vp, char *buf, int len, long *errp)
{
    Device_WriteParms p;
    Device_WriteResp *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    if (len > vp->dev_maxrecvsize)
        RETURN_ERR(errp, VXI11_ERR_TOOBIG, -1);
    p.lid                 = vp->dev_lid;
    p.io_timeout          = vp->dev_iotimeout;
    p.lock_timeout        = vp->dev_locktimeout;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.flags              |= vp->dev_eot      ? VXI11_FLAG_ENDW     : 0;
    p.data.data_val       = buf;
    p.data.data_len       = len;
    r = device_write_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error != 0)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, r->size);
}


int
vxi11_device_writeall(vxi11_device_t vp, char *buf, int len, long *errp)
{
    int n, count = 0;
    int try;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    do {
        try = MIN(len - count, vp->dev_maxrecvsize);
        n = vxi11_device_write(vp, buf + count, try, errp);
        if (n == -1)
            return -1;
        count += n;
    } while (count < len);
    RETURN_OK(errp, count);
}

int
vxi11_device_readstb(vxi11_device_t vp, unsigned char *statusp, long *errp)
{
    Device_GenericParms p;
    Device_ReadStbResp *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    p.lid                 = vp->dev_lid;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.lock_timeout        = vp->dev_locktimeout;
    p.io_timeout          = vp->dev_iotimeout;
    r = device_readstb_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    if (statusp)
        *statusp = r->stb;
    RETURN_OK(errp, 0);
}

typedef enum { GC_TRIGGER, GC_CLEAR, GC_REMOTE, GC_LOCAL } generic_cmd_t;

static int
_device_generic(vxi11_device_t vp, generic_cmd_t cmd, long *errp)
{
    Device_GenericParms p;
    Device_Error *r;

    p.lid                 = vp->dev_lid;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->dev_iotimeout;
    p.lock_timeout        = vp->dev_locktimeout;
    switch (cmd) {
        case GC_TRIGGER:
            r = device_trigger_1(&p, _chan_core(vp));
            break;
        case GC_CLEAR:
            r = device_clear_1(&p, _chan_core(vp));
            break;
        case GC_REMOTE:
            r = device_remote_1(&p, _chan_core(vp));
            break;
        case GC_LOCAL:
            r = device_local_1(&p, _chan_core(vp));
            break;
    }
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int
vxi11_device_trigger(vxi11_device_t vp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_generic(vp, GC_TRIGGER, errp);
}

int
vxi11_device_clear(vxi11_device_t vp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_generic(vp, GC_CLEAR, errp);
}

int
vxi11_device_remote(vxi11_device_t vp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_generic(vp, GC_REMOTE, errp);
}

int
vxi11_device_local(vxi11_device_t vp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_generic(vp, GC_LOCAL, errp);
}

int
vxi11_device_lock(vxi11_device_t vp, long *errp)
{
    Device_LockParms p;
    Device_Error *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    p.lid                 = vp->dev_lid;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.lock_timeout        = vp->dev_locktimeout;
    r = device_lock_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int
vxi11_device_unlock(vxi11_device_t vp, long *errp)
{
    Device_Error *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    r = device_unlock_1(&vp->dev_lid, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int
vxi11_device_docmd_sendcmds(vxi11_device_t vp, char *cmds, int len, long *errp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    if (len > 128)
        RETURN_ERR(errp, VXI11_ERR_PARAMETER, -1);
    p.lid                 = vp->dev_lid;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->dev_iotimeout;
    p.lock_timeout        = vp->dev_locktimeout;
    p.cmd                 = VXI11_DOCMD_SEND_COMMAND;
    p.network_order       = 1;
    p.datasize            = 1;
    p.data_in.data_in_val = cmds;
    p.data_in.data_in_len = len;
    r = device_docmd_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    if (r->data_out.data_out_len != len) /* rule B.5.5 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (memcmp(r->data_out.data_out_val, cmds, len) != 0) /* rule B.5.5 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    RETURN_OK(errp, 0);
}

/* helper for docmd_stat_*, docmd_atn, docmd_ren */
static int
_device_docmd_16(vxi11_device_t vp, int cmd, int in, int *outp, long *errp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;
    uint16_t a = htons((uint16_t)in);

    p.lid                 = vp->dev_lid;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->dev_iotimeout;
    p.lock_timeout        = vp->dev_locktimeout;
    p.cmd                 = cmd;
    p.network_order       = 1;
    p.datasize            = 2;
    p.data_in.data_in_val = (char *)&a;
    p.data_in.data_in_len = 2;
    r = device_docmd_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    if (r->data_out.data_out_len != 2) /* rule B.5.6, B.5.7, B.5.8 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (outp)
        *outp = ntohs(*(uint16_t *)r->data_out.data_out_val);
    RETURN_OK(errp, 0);
}

int
vxi11_device_docmd_stat_ren(vxi11_device_t vp, int *renp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_REMOTE, renp, errp);
}

int
vxi11_device_docmd_stat_srq(vxi11_device_t vp, int *srqp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_SRQ, srqp, errp);
}

int
vxi11_device_docmd_stat_ndac(vxi11_device_t vp, int *ndacp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_NDAC, ndacp, errp);
}

int
vxi11_device_docmd_stat_sysctl(vxi11_device_t vp, int *sysctlp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_SYS_CTRLR, sysctlp, errp);
}

int
vxi11_device_docmd_stat_inchg(vxi11_device_t vp, int *inchgp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_CTRLR_CHRG, inchgp, errp);
}

int
vxi11_device_docmd_stat_talk(vxi11_device_t vp, int *talkp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_TALKER, talkp, errp);
}

int
vxi11_device_docmd_stat_listen(vxi11_device_t vp, int *listenp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_LISTENER, listenp, errp);
}

int
vxi11_device_docmd_stat_addr(vxi11_device_t vp, int *addrp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_BUSADDR, addrp, errp);
}

int
vxi11_device_docmd_atn(vxi11_device_t vp, int atn, long *errp)
{
    int res, outarg;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    res = _device_docmd_16(vp, VXI11_DOCMD_ATN_CONTROL, atn, &outarg, errp);
    if (res == 0 && outarg != atn) /* rule B.5.7 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    return res;
}

int
vxi11_device_docmd_ren(vxi11_device_t vp, int ren, long *errp)
{
    int res, outarg;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    res = _device_docmd_16(vp, VXI11_DOCMD_REN_CONTROL, ren, &outarg, errp);
    if (res == 0 && outarg != ren) /* rule B.5.8 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    return res;
}

/* helper for docmd_pass, docmd_addr */
static int
_device_docmd_32(vxi11_device_t vp, int cmd, int in, int *outp, long *errp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;
    uint32_t a = htonl((uint32_t)in);

    p.lid                 = vp->dev_lid;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->dev_iotimeout;
    p.lock_timeout        = vp->dev_locktimeout;
    p.cmd                 = cmd;
    p.network_order       = 1;
    p.datasize            = 4;
    p.data_in.data_in_val = (char *)&a;
    p.data_in.data_in_len = 4;
    r = device_docmd_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    if (r->data_out.data_out_len != 4) /* rule B.5.9, B.5.10 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (outp)
        *outp = ntohl(*(uint32_t *)r->data_out.data_out_val);
    RETURN_OK(errp, 0);
}

int
vxi11_device_docmd_pass(vxi11_device_t vp, int addr, long *errp)
{
    int res, outarg;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    res = _device_docmd_32(vp, VXI11_DOCMD_PASS_CONTROL, addr, &outarg, errp);
    if (res == 0 && outarg != addr) /* rule B.5.9 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    return res;
}

int
vxi11_device_docmd_addr(vxi11_device_t vp, int addr, long *errp)
{
    int res, outarg;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    if (addr < 0 || addr > 30)
        RETURN_ERR(errp, VXI11_ERR_PARAMETER, -1);
    res = _device_docmd_32(vp, VXI11_DOCMD_BUS_ADDRESS, addr, &outarg, errp);
    if (res == 0 && outarg != addr) /* rule B.5.10 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    return res;
}

int
vxi11_device_docmd_ifc(vxi11_device_t vp, long *errp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    p.lid                 = vp->dev_lid;
    p.flags               = vp->dev_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->dev_iotimeout;
    p.lock_timeout        = vp->dev_locktimeout;
    p.cmd                 = VXI11_DOCMD_IFC_CONTROL;
    p.network_order       = 1;
    p.datasize            = 0;
    p.data_in.data_in_val = NULL;
    p.data_in.data_in_len = 0;
    r = device_docmd_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
#if 0
    /* ignore out of spec len=2 response from ics8065 */
    if (r->data_out.data_out_len != 0) /* rule B.5.11 */
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
#endif
    RETURN_OK(errp, 0);
}

int
vxi11_device_abort(vxi11_device_t vp, long *errp)
{
    Device_Error *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    if (vp->dev_abrt == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    r = device_abort_1(&vp->dev_lid, vp->dev_abrt);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->dev_abrt), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int
vxi11_create_intr_chan(vxi11_device_t vp, srq_fun_t fun, void *arg, long *errp)
{
    Device_RemoteFunc p;
    Device_Error *r;
    struct sockaddr_in sin;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    if (!(vp->dev_intr_svc = svctcp_create(RPC_ANYSOCK, 0, 0)))
        RETURN_ERR(errp, VXI11_ERR_SVCTCP, -1);
    if (!svc_register(vp->dev_intr_svc, VXI11_INTR_SVC_PROGNUM, 
                      VXI11_INTR_SVC_VERSION, device_intr_1, IPPROTO_TCP))
        RETURN_ERR(errp, VXI11_ERR_SVCREG, -1);
    vp->dev_intr_fun = fun;
    vp->dev_intr_arg = arg;
    p.hostAddr            = ntohl(vp->dev_intr_svc->xp_raddr.sin_addr.s_addr);
    p.hostPort            = ntohs(vp->dev_intr_svc->xp_port);
    p.progNum             = VXI11_INTR_SVC_PROGNUM; /* rule B.6.87 */
    p.progVers            = VXI11_INTR_SVC_VERSION; /* rule B.6.88 */
    p.progFamily          = DEVICE_TCP;
    r = create_intr_chan_1(&p, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int
vxi11_destroy_intr_chan(vxi11_device_t vp, long *errp)
{
    Device_Error *r;

    /* FIXME how to destroy the RPC service on this end? */

    /* destroy callback passed into create function?  it could pthread_signal
     * the service loop to terminate it?  
     */

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    r = destroy_intr_chan_1(NULL, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int 
vxi11_device_enable_srq(vxi11_device_t vp, int enable, long *errp)
{
    Device_EnableSrqParms p;
    Device_Error *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    p.lid                 = vp->dev_lid;
    p.enable              = enable;
    /* FIXME: encode vxi11_device_t -> p.handle */
    p.handle.handle_val   = NULL; /* not used */
    p.handle.handle_len   = 0;
    r = device_enable_srq_1(NULL, _chan_core(vp));
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

/* called from device_intr_1 in vxi11intr_svc.c */
void *
device_intr_srq_1_svc(Device_SrqParms *p, struct svc_req *req)
{
    printf("device_intr_srq_1_svc called\n");
    /* FIXME decode p->handle -> vxi11_device_t, then call dev_intr_fun */
    return NULL;
}

int
vxi11_intr_svc_run(vxi11_device_t vp, long *errp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    /* FIXME: check preconditions */
    svc_run();
    RETURN_ERR(errp, VXI11_ERR_SVCRET, -1);
}


void vxi11_set_eos(vxi11_device_t vp, unsigned char eos)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    vp->dev_eos = eos;
}

unsigned char vxi11_get_eos(vxi11_device_t vp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return vp->dev_eos;
}

void vxi11_set_reos(vxi11_device_t vp, int reos)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    vp->dev_reos = reos;
}

int vxi11_get_reos(vxi11_device_t vp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return vp->dev_reos;
}

void vxi11_set_eot(vxi11_device_t vp, int eot)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    vp->dev_eot = eot;
}

int vxi11_get_eot(vxi11_device_t vp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return vp->dev_eot;
}

void vxi11_set_waitlock(vxi11_device_t vp, int waitlock)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    vp->dev_waitlock = waitlock;
}

int vxi11_get_waitlock(vxi11_device_t vp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return vp->dev_waitlock;
}

void vxi11_set_locktimeout(vxi11_device_t vp, unsigned long msec)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    vp->dev_locktimeout = msec;
}

unsigned long vxi11_get_locktimeout(vxi11_device_t vp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return vp->dev_locktimeout;
}

void vxi11_set_iotimeout(vxi11_device_t vp, unsigned long msec)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    vp->dev_iotimeout = msec;
}

unsigned long vxi11_get_iotimeout(vxi11_device_t vp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return vp->dev_iotimeout;
}

unsigned long vxi11_get_maxrecvsize(vxi11_device_t vp)
{
    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    return vp->dev_maxrecvsize;
}

void 
vxi11_set_abrt_rpctimeout(vxi11_device_t vp, unsigned long msec)
{
    struct timeval tv;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec % 1000) * 1000;
    if (vp->dev_abrt)
        clnt_control(vp->dev_abrt, CLSET_TIMEOUT, (char *)&tv);
}

unsigned long 
vxi11_get_abrt_rpctimeout(vxi11_device_t vp)
{
    struct timeval tv = {0, 0};

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    if (vp->dev_abrt)
        clnt_control(vp->dev_abrt, CLGET_TIMEOUT, (char *)&tv);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/**
 ** Device link/unlink
 **/

static inline void 
_link_channel(vxi11_device_t vp)
{
    vp->dev_next = vp->dev_channel->chan_devices;
    vp->dev_prev = NULL;
    vp->dev_channel->chan_devices = vp;

}

static inline void 
_unlink_channel(vxi11_device_t vp)
{
    if (vp->dev_prev)
        (vp->dev_prev)->dev_next = vp->dev_next;
    else
        vp->dev_channel->chan_devices = vp->dev_next;
    if (vp->dev_next)
        (vp->dev_next)->dev_prev = vp->dev_prev;
}

int
vxi11_unlink(vxi11_device_t vp, long *errp)
{
    Device_Error *r;

    assert(vp->dev_magic == VXI11_DEVICE_MAGIC);
    if (vp->dev_abrt)
        clnt_destroy(vp->dev_abrt);
    if (vp->dev_lid != -1)
        /* error */
    r = destroy_link_1(&vp->dev_lid, _chan_core(vp));
    /* handle error */
    _unlink_channel(vp);
    memset(vp, 0, sizeof(struct vxi11_device_struct));
    free(vp);

}

vxi11_device_t
vxi11_link(vxi11_channel_t cp, char *device, int lockdevice, 
           unsigned long locktimeout, int doabort, long *errp)
{
    vxi11_device_t vp;
    Create_LinkParms p;
    Create_LinkResp *r;

    vp = malloc(sizeof(struct vxi11_device_struct));
    if (!vp)
        RETURN_ERR(errp, VXI11_ERR_RESOURCES, NULL);
    vp->dev_magic       = VXI11_DEVICE_MAGIC;
    vp->dev_eos         = VXI11_DEFAULT_EOS;
    vp->dev_reos        = VXI11_DEFAULT_REOS;
    vp->dev_eot         = VXI11_DEFAULT_EOT;
    vp->dev_waitlock    = lockdevice;
    vp->dev_locktimeout = lockdevice ? locktimeout : VXI11_DEFAULT_LOCKTIMEOUT;
    vp->dev_iotimeout   = VXI11_DEFAULT_IOTIMEOUT;
    vp->dev_channel     = cp;
    vp->dev_abrt        = NULL;
    vp->dev_intr_svc    = NULL;

    p.device = device;
    p.lockDevice = vp->dev_waitlock;
    p.lock_timeout = vp->dev_locktimeout;
    p.clientId = 0; /* not used */
    r = create_link_1(&p, _chan_core(vp));
    if (r == NULL) {
        vxi11_unlink(vp);
        RETURN_ERR(errp, _rpcerr(_chan_core(vp)), NULL);
    }
    if (r->error) {
        vxi11_unlink(vp);
        RETURN_ERR(errp, r->error, NULL);
    }
    vp->dev_lid = r->lid;
    if (r->maxRecvSize < 1024) { /* observation B.6.5 / rule B.6.3 */
        vxi11_unlink(vp);
        RETURN_ERR(errp, VXI11_ERR_BADRESP, NULL);
    }
    vp->dev_maxrecvsize = r->maxRecvSize;

    if (doabort) {
        struct sockaddr_in sin;
        int sock = RPC_ANYSOCK;

        clnt_control(_chan_core(vp), CLGET_SERVER_ADDR, (char *)&sin);
        sin.sin_port = htons(r->abortPort);
        vp->dev_abrt = clnttcp_create(&sin, DEVICE_ASYNC, 
                                          DEVICE_ASYNC_VERSION, &sock, 0, 0);
        if (vp->dev_abrt == NULL) {
            vxi11_unlink(vp);
            RETURN_ERR(errp, _rpcerr(NULL), NULL);
        } 
        if (rpctimeout > 0)
            vxi11_set_abrt_rpctimeout(vp, rpctimeout);
    }

    _link_channel(vp);
    RETURN_OK(errp, vp);
}

/**
 ** Channel operations 
 **/

void 
vxi11_set_core_rpctimeout(vxi11_channel_t cp, unsigned long msec)
{
    struct timeval tv;

    assert(cp->chan_magic == VXI11_CHANNEL_MAGIC);
    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec % 1000) * 1000;
    if (cp->chan_core)
        clnt_control(cp->chan_core, CLSET_TIMEOUT, (char *)&tv);
}

unsigned long 
vxi11_get_core_rpctimeout(vxi11_channel_t cp)
{
    struct timeval tv = {0, 0};

    assert(cp->chan_magic == VXI11_CHANNEL_MAGIC);
    if (cp->chan_core)
        clnt_control(cp->chan_core, CLGET_TIMEOUT, (char *)&tv);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int
vxi11_close(vxi11_channel_t cp, long *errp)
{
    vxi11_device_t *vp;
    long errp;

    assert(cp->chan_magic == VXI11_CHANNEL_MAGIC);
    if (cp->chan_devices != NULL)
        RETURN_ERR(errp, VXI11_ERR_HASLINKS);
    if (cp->chan_core)
        clnt_destroy(cp->chan_core);
    memset(cp, 0, sizeof(struct vxi11_channel_struct));
    free(cp);
}

vxi11_channel_t
vxi11_open(char *host, long *errp)
{
    vxi11_channel_t cp;

    cp = malloc(sizeof(struct vxi11_channel_struct));
    if (!cp)
        RETURN_ERR(errp, VXI11_ERR_RESOURCES, NULL);
    cp->chan_magic = VXI11_CHANNEL_MAGIC;
    cp->chan_devices = NULL;
    cp->chan_core = clnt_create(host, DEVICE_CORE, DEVICE_CORE_VERSION, "tcp");
    if (cp->chan_core == NULL) {
        vxi11_close(cp);
        RETURN_ERR(errp, _rpcerr(NULL), NULL);
    }
    RETURN_OK(errp, cp);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
