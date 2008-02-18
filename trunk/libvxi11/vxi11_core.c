/*
 * vxi11core.c - wrappers for core VXI-11 functions
 */

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
#include "vxi11_core.h"
#include "rpccache.h"

static int vxi11_core_debug = 0;

int
vxi11_open_core_channel(char *host, CLIENT **corep)
{
    CLIENT *core;
    int res = VXI11_CORE_CREATE;

    core = clnt_create_cached(host, DEVICE_CORE, DEVICE_CORE_VERSION, "tcp");
    if (core) {
        if (corep)
            *corep = core;
        res = 0;
    }
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_open_core_channel (host=%s, ...) = %d "
                "core=%p\n", host, res, core);
    return res;
}

void
vxi11_close_core_channel(CLIENT *core)
{
    clnt_destroy_cached(core);
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_close_core_channel (core=%p)\n", core);
}

int 
vxi11_open_abrt_channel(CLIENT *core, unsigned short abortPort, CLIENT **abrtp)
{
    struct sockaddr_in sin;
    int sock = RPC_ANYSOCK;
    CLIENT *abrt;
    int res = VXI11_ABRT_CREATE;

    clnt_control(core, CLGET_SERVER_ADDR, (char *)&sin);
    sin.sin_port = htons(abortPort);
    if ((abrt = clnttcp_create_cached(&sin, DEVICE_ASYNC, 
                                       DEVICE_ASYNC_VERSION, &sock, 0, 0))) {
        if (abrtp)
            *abrtp = abrt;
        res = 0;
    }
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_open_abrt_channel (core=%p "
                "abortPort=%d, ...) = %d abrt=%p\n", 
                core, abortPort, res, abrt);
    return res;
}

void
vxi11_close_abrt_channel(CLIENT *abrt)
{
    clnt_destroy_cached(abrt);
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_close_abrt_channel (abrt=%p)\n", abrt);
}

int
vxi11_create_link(CLIENT *core, int clientId, int lockDevice, 
                  unsigned long lock_timeout, char *device, long *lidp, 
                  unsigned short *abortPortp, unsigned long *maxRecvSizep)
{
    Create_LinkParms p;
    Create_LinkResp *r;
    int res = VXI11_CORE_RPCERR;

    p.clientId = clientId;
    p.lockDevice = lockDevice;
    p.lock_timeout = lock_timeout;
    p.device = device;
    if ((r = create_link_1(&p, core))) {
        if (lidp)
            *lidp = r->lid;
        if (abortPortp)
            *abortPortp = r->abortPort;
        if (maxRecvSizep)
            *maxRecvSizep = r->maxRecvSize;
        res = r->error;
    }
    if (vxi11_core_debug) {
        fprintf(stderr, "XXX vxi11_create_link (core=%p, clientId=%d, "
                "lockDevice=%d, lock_timeout=%lu, device=%s, ...) = %d ",
                core, clientId, lockDevice, lock_timeout, device, res);
        if (r)
            fprintf(stderr, "lid=%ld, abortPort=%d, maxRecvSize=%lu\n",
                    r->lid, r->abortPort, r->maxRecvSize);
        else
            fprintf(stderr, "\n");
    }
    return res;
}

int
vxi11_device_write(CLIENT *core, long lid, long flags,
                   unsigned long io_timeout, unsigned long lock_timeout,
                   char *data_val, int data_len, unsigned long *sizep)
{
    Device_WriteParms p;
    Device_WriteResp *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.io_timeout = io_timeout;
    p.lock_timeout = lock_timeout;
    p.flags = flags;
    p.data.data_val = data_val;
    p.data.data_len = data_len;
    if ((r = device_write_1(&p, core))) {
        if (sizep)
            *sizep = r->size;
        res = r->error;
    }
    if (vxi11_core_debug) {
        fprintf(stderr, "XXX vxi11_device_write (core=%p, lid=%ld, "
                "flags=%ld io_timeout=%lu, lock_timeout=%lu, "
                "..., data_len=%d, ...) = %d ",
                core, lid, flags, io_timeout, lock_timeout, data_len, res);
        if (r)
            fprintf(stderr, "size=%lu\n", r->size);
        else
            fprintf(stderr, "\n");
    }
    return res;
}

int
vxi11_device_read(CLIENT *core, long lid, long flags, 
                  unsigned long io_timeout, unsigned long lock_timeout, 
                  int termChar, int *reasonp, 
                  char *data_val, int *data_lenp, unsigned long requestSize)
{
    Device_ReadParms p;
    Device_ReadResp *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.requestSize = requestSize;
    p.io_timeout = io_timeout;
    p.lock_timeout = lock_timeout;
    p.flags = flags;
    p.termChar = termChar;
    if ((r = device_read_1(&p, core))) {
        if (reasonp)
            *reasonp = r->reason;
        if (data_lenp)
            *data_lenp = r->data.data_len;
        if (data_val && r->data.data_val)
            memcpy(data_val, r->data.data_val, r->data.data_len);
        res = r->error;
    }
    if (vxi11_core_debug) {
        fprintf(stderr, "XXX vxi11_device_read (core=%p, lid=%ld, "
                "flags=%ld io_timeout=%lu, lock_timeout=%lu, "
                "termChar=0x%x, ..., requestSize=%lu) = %d ",
                core, lid, flags, io_timeout, lock_timeout, termChar,
                requestSize, res);
        if (r) 
            fprintf(stderr, "reason=%ld, data_len=%d\n",
                    r->reason, r->data.data_len);
        else
            fprintf(stderr, "\n");
    }
    return res;
}

int
vxi11_device_readstb(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout,
                     unsigned char *stbp)
{
    Device_GenericParms p;
    Device_ReadStbResp *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if ((r = device_readstb_1(&p, core))) {
        if (stbp)
            *stbp = r->stb;
        res = r->error;
    }
    if (vxi11_core_debug) {
        fprintf(stderr, "XXX vxi11_device_readstb (core=%p, lid=%ld, "
                "flags=%ld io_timeout=%lu, lock_timeout=%lu, ...) = %d ",
                core, lid, flags, io_timeout, lock_timeout, res);
        if (r)
            fprintf(stderr, "stb=0x%x\n", r->stb);
        else
            fprintf(stderr, "\n");
    }
    return res;
}

int
vxi11_device_trigger(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout)
{
    Device_GenericParms p;
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if ((r = device_trigger_1(&p, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_trigger (core=%p, lid=%ld, "
                "flags=%ld io_timeout=%lu, lock_timeout=%lu) = %d\n",
                core, lid, flags, io_timeout, lock_timeout, res);
    return res;
}

int
vxi11_device_clear(CLIENT *core, long lid, long flags, 
                     unsigned long io_timeout, unsigned long lock_timeout)
{
    Device_GenericParms p;
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if ((r = device_clear_1(&p, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_clear (core=%p, lid=%ld, "
                "flags=%ld io_timeout=%lu, lock_timeout=%lu) = %d\n",
                core, lid, flags, io_timeout, lock_timeout, res);
    return res;
}

int
vxi11_device_remote(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout)
{
    Device_GenericParms p;
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if ((r = device_remote_1(&p, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_remote (core=%p, lid=%ld, "
                "flags=%ld io_timeout=%lu, lock_timeout=%lu) = %d\n",
                core, lid, flags, io_timeout, lock_timeout, res);
    return res;
}

int
vxi11_device_local(CLIENT *core, long lid, long flags,
                   unsigned long io_timeout, unsigned long lock_timeout)
{
    Device_GenericParms p;
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if ((r = device_local_1(&p, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_local (core=%p, lid=%ld, "
                "flags=%ld io_timeout=%lu, lock_timeout=%lu) = %d\n",
                core, lid, flags, io_timeout, lock_timeout, res);
    return res;
}

int
vxi11_device_lock(CLIENT *core, long lid, long flags, 
                  unsigned long lock_timeout)
{
    Device_LockParms p;
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    if ((r = device_lock_1(&p, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_lock (core=%p, lid=%ld, "
                "flags=%ld lock_timeout=%lu) = %d\n",
                core, lid, flags, lock_timeout, res);
    return res;
}

int
vxi11_device_unlock(CLIENT *core, long lid)
{
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    if ((r = device_unlock_1(&lid, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_unlock (core=%p, lid=%ld) = %d\n",
                core, lid, res);
    return res;
}

int
vxi11_device_enable_srq(CLIENT *core, long lid, int enable, char *handle_val,
                        int handle_len)
{
    Device_EnableSrqParms p;
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.enable = enable;
    p.handle.handle_val = handle_val;
    p.handle.handle_len = handle_len; /* XXX max 40 */
    if ((r = device_enable_srq_1(&p, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_enable_srq (core=%p, lid=%ld "
                "enable=%d, ..., handle_len=%d) = %d\n",
                core, lid, enable, handle_len, res);
    return res;
}

int
vxi11_device_docmd(CLIENT *core, long lid, long flags, 
                   unsigned long io_timeout, unsigned long lock_timeout,
                   long cmd, int network_order, long datasize, 
                   char *data_in_val, int data_in_len,
                   char **data_out_valp, int *data_out_lenp)
{
    Device_DocmdParms p;
    Device_DocmdResp *r;
    int res = VXI11_CORE_RPCERR;

    p.lid = lid;
    p.flags = flags;
    p.io_timeout = io_timeout;
    p.lock_timeout = lock_timeout;
    p.cmd = cmd;
    p.network_order = network_order;
    p.datasize = datasize;
    p.data_in.data_in_val = data_in_val;
    p.data_in.data_in_len = data_in_len;
    if ((r = device_docmd_1(&p, core))) {
        if (data_out_valp)
            *data_out_valp = r->data_out.data_out_val;
        if (data_out_lenp)
            *data_out_lenp = r->data_out.data_out_len;
        res = r->error;
    }
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_docmd (core=%p, lid=%ld, "
                "flags=%ld, io_timeout=%lu, lock_timeout=%lu, cmd=%ld, "
                "network_order=%d, datasize=%ld, ...,  data_in_len=%d, "
                " ...) = %d\n",
                core, lid, flags, io_timeout, lock_timeout, cmd,
                network_order, datasize, data_in_len, res);
    return res;
}

int
vxi11_destroy_link(CLIENT *core, long lid)
{
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    if ((r = destroy_link_1(&lid, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_destroy_link (core=%p, lid=%ld) = %d\n",
                core, lid, res);
    return res;
}

int
vxi11_create_intr_chan(CLIENT *core, unsigned long hostAddr,
                       unsigned short hostPort, unsigned long progNum,
                       unsigned long progVers, int progFamily)
{
    Device_RemoteFunc p;
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    p.hostAddr = hostAddr;
    p.hostPort = hostPort;
    p.progNum = progNum;
    p.progVers = progVers;
    p.progFamily = progFamily;
    if ((r = create_intr_chan_1(&p, core)))
        res = r->error;
    if (vxi11_core_debug) {
        struct in_addr addr;
        addr.s_addr = htonl(hostAddr);
        fprintf(stderr, "XXX vxi11_create_intr_chan (core=%p, "
                "hostAddr=%s, hostPort=%d, progNum=%lu, progVers=%lu, "
                "progFamily=%d) = %d\n", core, inet_ntoa(addr), 
                hostPort, progNum, progVers, progFamily, res);
    }
    return res;
}

int
vxi11_destroy_intr_chan(CLIENT *core)
{
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    if ((r = destroy_intr_chan_1(NULL, core)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_destroy_intr_chan (core=%p) = %d\n", 
                core, res);
    return res;
}

int
vxi11_device_abort(CLIENT *abrt, long lid)
{
    Device_Error *r;
    int res = VXI11_CORE_RPCERR;

    if ((r = device_abort_1(&lid, abrt)))
        res = r->error;
    if (vxi11_core_debug)
        fprintf(stderr, "XXX vxi11_device_abort (abrt=%p, lid=%ld) = %d\n", 
                abrt, lid, res);
    return res;
}

void
vxi11_set_core_debug(int doDebug)
{
    vxi11_core_debug = doDebug;
    set_rpccache_debug(doDebug);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
