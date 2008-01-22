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

#define VXI11_DEFAULT_EOS         0x0c
#define VXI11_DEFAULT_REOS        1
#define VXI11_DEFAULT_EOT         1
#define VXI11_DEFAULT_LOCKTIMEOUT 30000 /* 30s */
#define VXI11_DEFAULT_IOTIMEOUT   30000 /* 30s */

#define VXI11_HANDLE_MAGIC  0x4c4ba309
struct vxi11_handle_struct {
    int             vxi_magic;
    CLIENT         *vxi_core_cli;   /* core channel */
    CLIENT         *vxi_abrt_cli;   /* abort channel */
    Device_Link     vxi_lid;        /* device link */
    unsigned long   vxi_maxrecvsize;/* max write device will accept */
    unsigned char   vxi_eos;        /* termchar used in read */
    int             vxi_reos;       /* if true, termchar can terminate read */
    int             vxi_eot;        /* if true, assert EOI on write */
    int             vxi_waitlock;   /* if true, block if device locked */
    unsigned long   vxi_locktimeout;/* lock timeout in milliseconds */
    unsigned long   vxi_iotimeout;  /* io timeout in milliseconds */
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
vxi11_device_read(vxi11_handle_t vp, char *buf, int len, 
                  int *reasonp, long *errp)
{
    Device_ReadParms p;
    Device_ReadResp *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    p.lid                 = vp->vxi_lid;
    p.requestSize         = len;
    p.io_timeout          = vp->vxi_iotimeout;
    p.lock_timeout        = vp->vxi_locktimeout;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK   : 0;
    p.flags              |= vp->vxi_reos     ? VXI11_FLAG_TERMCHRSET : 0;
    p.termChar            = vp->vxi_eos;
    r = device_read_1(&p, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error != 0)
        RETURN_ERR(errp, r->error, -1);
    if (r->data.data_len > len)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (r->data.data_len == len && !(r->reason & VXI11_REASON_REQCNT))
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (r->data.data_len < len && (r->reason & VXI11_REASON_REQCNT))
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (!vp->vxi_reos && (r->reason & VXI11_REASON_CHR))
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    memcpy(buf, r->data.data_val, r->data.data_len);
    if (reasonp)
        *reasonp = r->reason;
    RETURN_OK(errp, r->data.data_len);
    /* observation B.6.9: short read possible (reason == 0 and error == 0) */
}

int
vxi11_device_readall(vxi11_handle_t vp, char *buf, int len, long *errp)
{
    int n, count = 0;
    int reason, try; 

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    do {
        try = MIN(len - count, 1024); /* rule B.6.3: client must accept 1024 */
        n = vxi11_device_read(vp, buf + count, try, &reason, errp);
        if (n == -1)
            return -1;
        count += n;
    } while (reason == 0);
    RETURN_OK(errp, count);
}

int
vxi11_device_write(vxi11_handle_t vp, char *buf, int len, long *errp)
{
    Device_WriteParms p;
    Device_WriteResp *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    if (len > vp->vxi_maxrecvsize)
        RETURN_ERR(errp, VXI11_ERR_TOOBIG, -1);
    p.lid                 = vp->vxi_lid;
    p.io_timeout          = vp->vxi_iotimeout;
    p.lock_timeout        = vp->vxi_locktimeout;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.flags              |= vp->vxi_eot      ? VXI11_FLAG_ENDW     : 0;
    p.data.data_val       = buf;
    p.data.data_len       = len;
    r = device_write_1(&p, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error != 0)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, r->size);
}


int
vxi11_device_writeall(vxi11_handle_t vp, char *buf, int len, long *errp)
{
    int n, count = 0;
    int try;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    do {
        try = MIN(len - count, vp->vxi_maxrecvsize);
        n = vxi11_device_write(vp, buf + count, try, errp);
        if (n == -1)
            return -1;
        count += n;
    } while (count < len);
    RETURN_OK(errp, count);
}

int
vxi11_device_readstb(vxi11_handle_t vp, unsigned char *statusp, long *errp)
{
    Device_GenericParms p;
    Device_ReadStbResp *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    p.lid                 = vp->vxi_lid;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.lock_timeout        = vp->vxi_locktimeout;
    p.io_timeout          = vp->vxi_iotimeout;
    r = device_readstb_1(&p, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    if (statusp)
        *statusp = r->stb;
    RETURN_OK(errp, 0);
}

typedef enum { GC_TRIGGER, GC_CLEAR, GC_REMOTE, GC_LOCAL } generic_cmd_t;

static int
_device_generic(vxi11_handle_t vp, generic_cmd_t cmd, long *errp)
{
    Device_GenericParms p;
    Device_Error *r;

    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    p.lid                 = vp->vxi_lid;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->vxi_iotimeout;
    p.lock_timeout        = vp->vxi_locktimeout;
    switch (cmd) {
        case GC_TRIGGER:
            r = device_trigger_1(&p, vp->vxi_core_cli);
            break;
        case GC_CLEAR:
            r = device_clear_1(&p, vp->vxi_core_cli);
            break;
        case GC_REMOTE:
            r = device_remote_1(&p, vp->vxi_core_cli);
            break;
        case GC_LOCAL:
            r = device_local_1(&p, vp->vxi_core_cli);
            break;
    }
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int
vxi11_device_trigger(vxi11_handle_t vp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_generic(vp, GC_TRIGGER, errp);
}

int
vxi11_device_clear(vxi11_handle_t vp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_generic(vp, GC_CLEAR, errp);
}

int
vxi11_device_remote(vxi11_handle_t vp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_generic(vp, GC_REMOTE, errp);
}

int
vxi11_device_local(vxi11_handle_t vp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_generic(vp, GC_LOCAL, errp);
}

int
vxi11_device_lock(vxi11_handle_t vp, long *errp)
{
    Device_LockParms p;
    Device_Error *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    p.lid                 = vp->vxi_lid;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.lock_timeout        = vp->vxi_locktimeout;
    r = device_lock_1(&p, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int
vxi11_device_unlock(vxi11_handle_t vp, long *errp)
{
    Device_Error *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    r = device_unlock_1(&vp->vxi_lid, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

int
vxi11_device_docmd_sendcmds(vxi11_handle_t vp, char *cmds, int len, long *errp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    if (len > 128)
        RETURN_ERR(errp, VXI11_ERR_PARAMETER, -1);
    p.lid                 = vp->vxi_lid;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->vxi_iotimeout;
    p.lock_timeout        = vp->vxi_locktimeout;
    p.cmd                 = VXI11_DOCMD_SEND_COMMAND;
    p.network_order       = 1;
    p.datasize            = 1;
    p.data_in.data_in_val = cmds;
    p.data_in.data_in_len = len;
    r = device_docmd_1(&p, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    if (r->data_out.data_out_len != len)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (memcmp(r->data_out.data_out_val, cmds, len) != 0)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    RETURN_OK(errp, 0);
}

/* helper for docmd_stat_*, docmd_atn, docmd_ren */
static int
_device_docmd_16(vxi11_handle_t vp, int cmd, int in, int *outp, long *errp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;
    uint16_t a = htons((uint16_t)in);

    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    p.lid                 = vp->vxi_lid;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->vxi_iotimeout;
    p.lock_timeout        = vp->vxi_locktimeout;
    p.cmd                 = cmd;
    p.network_order       = 1;
    p.datasize            = 2;
    p.data_in.data_in_val = (char *)&a;
    p.data_in.data_in_len = 2;
    r = device_docmd_1(&p, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    if (r->data_out.data_out_len != 2)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (outp)
        *outp = ntohs(*(uint16_t *)r->data_out.data_out_val);
    RETURN_OK(errp, 0);
}

int
vxi11_device_docmd_stat_ren(vxi11_handle_t vp, int *renp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_REMOTE, renp, errp);
}

int
vxi11_device_docmd_stat_srq(vxi11_handle_t vp, int *srqp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_SRQ, srqp, errp);
}

int
vxi11_device_docmd_stat_ndac(vxi11_handle_t vp, int *ndacp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_NDAC, ndacp, errp);
}

int
vxi11_device_docmd_stat_sysctl(vxi11_handle_t vp, int *sysctlp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_SYS_CTRLR, sysctlp, errp);
}

int
vxi11_device_docmd_stat_inchg(vxi11_handle_t vp, int *inchgp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_CTRLR_CHRG, inchgp, errp);
}

int
vxi11_device_docmd_stat_talk(vxi11_handle_t vp, int *talkp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_TALKER, talkp, errp);
}

int
vxi11_device_docmd_stat_listen(vxi11_handle_t vp, int *listenp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_LISTENER, listenp, errp);
}

int
vxi11_device_docmd_stat_addr(vxi11_handle_t vp, int *addrp, long *errp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return _device_docmd_16(vp, VXI11_DOCMD_BUS_STATUS, 
                            VXI11_DOCMD_STAT_BUSADDR, addrp, errp);
}

int
vxi11_device_docmd_atn(vxi11_handle_t vp, int atn, long *errp)
{
    int res, outarg;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    res = _device_docmd_16(vp, VXI11_DOCMD_ATN_CONTROL, atn, &outarg, errp);
    if (res == 0 && outarg != atn)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    return res;
}

int
vxi11_device_docmd_ren(vxi11_handle_t vp, int ren, long *errp)
{
    int res, outarg;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    res = _device_docmd_16(vp, VXI11_DOCMD_REN_CONTROL, ren, &outarg, errp);
    if (res == 0 && outarg != ren)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    return res;
}

/* helper for docmd_pass, docmd_addr */
static int
_device_docmd_32(vxi11_handle_t vp, int cmd, int in, int *outp, long *errp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;
    uint32_t a = htonl((uint32_t)in);

    if (vp->vxi_core_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    p.lid                 = vp->vxi_lid;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->vxi_iotimeout;
    p.lock_timeout        = vp->vxi_locktimeout;
    p.cmd                 = cmd;
    p.network_order       = 1;
    p.datasize            = 4;
    p.data_in.data_in_val = (char *)&a;
    p.data_in.data_in_len = 4;
    r = device_docmd_1(&p, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    if (r->data_out.data_out_len != 4)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    if (outp)
        *outp = ntohl(*(uint32_t *)r->data_out.data_out_val);
    RETURN_OK(errp, 0);
}

int
vxi11_device_docmd_pass(vxi11_handle_t vp, int addr, long *errp)
{
    int res, outarg;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    res = _device_docmd_32(vp, VXI11_DOCMD_PASS_CONTROL, addr, &outarg, errp);
    if (res == 0 && outarg != addr)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    return res;
}

int
vxi11_device_docmd_addr(vxi11_handle_t vp, int addr, long *errp)
{
    int res, outarg;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (addr < 0 || addr > 30)
        RETURN_ERR(errp, VXI11_ERR_PARAMETER, -1);
    res = _device_docmd_32(vp, VXI11_DOCMD_BUS_ADDRESS, addr, &outarg, errp);
    if (res == 0 && outarg != addr)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
    return res;
}

int
vxi11_device_docmd_ifc(vxi11_handle_t vp, long *errp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL) {
        if (errp)
            *errp = VXI11_ERR_NOCHAN;
        return -1;
    }
    p.lid                 = vp->vxi_lid;
    p.flags               = vp->vxi_waitlock ? VXI11_FLAG_WAITLOCK : 0;
    p.io_timeout          = vp->vxi_iotimeout;
    p.lock_timeout        = vp->vxi_locktimeout;
    p.cmd                 = VXI11_DOCMD_IFC_CONTROL;
    p.network_order       = 1;
    p.datasize            = 0;
    p.data_in.data_in_val = NULL;
    p.data_in.data_in_len = 0;
    r = device_docmd_1(&p, vp->vxi_core_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
#if 0
    /* ignore out of spec len=2 response from ics8065 */
    if (r->data_out.data_out_len != 0)
        RETURN_ERR(errp, VXI11_ERR_BADRESP, -1);
#endif
    RETURN_OK(errp, 0);
}

int
vxi11_device_abort(vxi11_handle_t vp, long *errp)
{
    Device_Error *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_abrt_cli == NULL)
        RETURN_ERR(errp, VXI11_ERR_NOCHAN, -1);
    r = device_abort_1(&vp->vxi_lid, vp->vxi_abrt_cli);
    if (r == NULL)
        RETURN_ERR(errp, _rpcerr(vp->vxi_abrt_cli), -1);
    if (r->error)
        RETURN_ERR(errp, r->error, -1);
    RETURN_OK(errp, 0);
}

void
vxi11_close(vxi11_handle_t vp)
{
    Device_Error *r;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_abrt_cli)
        clnt_destroy(vp->vxi_abrt_cli);
    if (vp->vxi_lid != -1 && vp->vxi_core_cli)
        r = destroy_link_1(&vp->vxi_lid, vp->vxi_core_cli); /* ignore errors */
    if (vp->vxi_core_cli)
        clnt_destroy(vp->vxi_core_cli);
    memset(vp, 0, sizeof(struct vxi11_handle_struct));
    free(vp);
}

vxi11_handle_t
vxi11_open(char *host, char *device, 
           int lockdevice, unsigned long locktimeout, unsigned long rpctimeout,
           int doabort, long *errp)
{
    vxi11_handle_t vp;
    Create_LinkParms p;
    Create_LinkResp *r;

    vp = malloc(sizeof(struct vxi11_handle_struct));
    if (!vp)
        RETURN_ERR(errp, VXI11_ERR_RESOURCES, NULL);
    vp->vxi_magic       = VXI11_HANDLE_MAGIC;
    vp->vxi_eos         = VXI11_DEFAULT_EOS;
    vp->vxi_reos        = VXI11_DEFAULT_REOS;
    vp->vxi_eot         = VXI11_DEFAULT_EOT;
    vp->vxi_waitlock    = lockdevice;
    vp->vxi_locktimeout = lockdevice ? locktimeout : VXI11_DEFAULT_LOCKTIMEOUT;
    vp->vxi_iotimeout   = VXI11_DEFAULT_IOTIMEOUT;
    vp->vxi_core_cli    = NULL;
    vp->vxi_abrt_cli    = NULL;

    /* open core connection */
    vp->vxi_core_cli    = clnt_create(host, DEVICE_CORE, DEVICE_CORE_VERSION, 
                                      "tcp");
    if (vp->vxi_core_cli == NULL) {
        vxi11_close(vp);
        RETURN_ERR(errp, _rpcerr(NULL), NULL);
    }
    if (rpctimeout > 0)
        vxi11_set_rpctimeout(vp, rpctimeout);

    /* establish link to instrument */
    p.device = device;
    p.lockDevice = vp->vxi_waitlock;
    p.lock_timeout = vp->vxi_locktimeout;
    p.clientId = 0; /* not used */
    r = create_link_1(&p, vp->vxi_core_cli);
    if (r == NULL) {
        vxi11_close(vp);
        RETURN_ERR(errp, _rpcerr(vp->vxi_core_cli), NULL);
    }
    vp->vxi_lid         = r->lid;
    vp->vxi_maxrecvsize = r->maxRecvSize;

    /* open abrt connection */
    if (doabort) {
        struct sockaddr_in addr;
        int sock = RPC_ANYSOCK;

        clnt_control(vp->vxi_core_cli, CLGET_SERVER_ADDR, (char *)&addr);
        addr.sin_port = htons(r->abortPort);
        vp->vxi_abrt_cli = clnttcp_create(&addr, DEVICE_ASYNC, 
                                          DEVICE_ASYNC_VERSION, &sock, 0, 0);
        if (vp->vxi_abrt_cli == NULL) {
            vxi11_close(vp);
            RETURN_ERR(errp, _rpcerr(NULL), NULL);
        } 
        if (rpctimeout > 0)
            vxi11_set_rpctimeout(vp, rpctimeout);
    }

    RETURN_OK(errp, vp);
}

void vxi11_set_eos(vxi11_handle_t vp, unsigned char eos)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    vp->vxi_eos = eos;
}

unsigned char vxi11_get_eos(vxi11_handle_t vp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return vp->vxi_eos;
}

void vxi11_set_reos(vxi11_handle_t vp, int reos)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    vp->vxi_reos = reos;
}

int vxi11_get_reos(vxi11_handle_t vp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return vp->vxi_reos;
}

void vxi11_set_eot(vxi11_handle_t vp, int eot)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    vp->vxi_eot = eot;
}

int vxi11_get_eot(vxi11_handle_t vp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return vp->vxi_eot;
}

void vxi11_set_waitlock(vxi11_handle_t vp, int waitlock)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    vp->vxi_waitlock = waitlock;
}

int vxi11_get_waitlock(vxi11_handle_t vp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return vp->vxi_waitlock;
}

void vxi11_set_locktimeout(vxi11_handle_t vp, unsigned long msec)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    vp->vxi_locktimeout = msec;
}

unsigned long vxi11_get_locktimeout(vxi11_handle_t vp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return vp->vxi_locktimeout;
}

void vxi11_set_iotimeout(vxi11_handle_t vp, unsigned long msec)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    vp->vxi_iotimeout = msec;
}

unsigned long vxi11_get_iotimeout(vxi11_handle_t vp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return vp->vxi_iotimeout;
}

void vxi11_set_rpctimeout(vxi11_handle_t vp, unsigned long msec)
{
    struct timeval tv;

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec % 1000) * 1000;
    if (vp->vxi_core_cli)
        clnt_control(vp->vxi_core_cli, CLSET_TIMEOUT, (char *)&tv);
    if (vp->vxi_abrt_cli)
        clnt_control(vp->vxi_abrt_cli, CLSET_TIMEOUT, (char *)&tv);
}

unsigned long vxi11_get_rpctimeout(vxi11_handle_t vp)
{
    struct timeval tv = {0, 0};

    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    if (vp->vxi_core_cli)
        clnt_control(vp->vxi_core_cli, CLGET_TIMEOUT, (char *)&tv);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

unsigned long vxi11_get_maxrecvsize(vxi11_handle_t vp)
{
    assert(vp->vxi_magic == VXI11_HANDLE_MAGIC);
    return vp->vxi_maxrecvsize;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
