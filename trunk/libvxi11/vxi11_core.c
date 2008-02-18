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

int
vxi11_open_core_channel(char *host, CLIENT **corep)
{
    CLIENT *core;

    core = clnt_create_cached(host, DEVICE_CORE, DEVICE_CORE_VERSION, "tcp");
    if (!core)
        return VXI11_CREATE_RPCERR;
    if (corep)
        *corep = core;
    return 0;
}

void
vxi11_close_core_channel(CLIENT *core)
{
    clnt_destroy_cached(core);
}

int 
vxi11_open_abrt_channel(CLIENT *core, unsigned short abortPort, CLIENT **abrtp)
{
    struct sockaddr_in sin;
    int sock = RPC_ANYSOCK;
    CLIENT *abrt;

    clnt_control(core, CLGET_SERVER_ADDR, (char *)&sin);
    sin.sin_port = htons(abortPort);
    if (!(abrt = clnttcp_create(&sin, DEVICE_ASYNC, DEVICE_ASYNC_VERSION, 
                                &sock, 0, 0)))
        return VXI11_CREATE_RPCERR;
    if (abrtp)
        *abrtp = abrt;
    return 0;
}

void
vxi11_close_abrt_channel(CLIENT *abrt)
{
    clnt_destroy(abrt);
}

int
vxi11_create_link(CLIENT *core, int clientId, int lockDevice, 
                  unsigned long lock_timeout, char *device, long *lidp, 
                  unsigned short *abortPortp, unsigned long *maxRecvSizep)
{
    Create_LinkParms p;
    Create_LinkResp *r;

    p.clientId = clientId;
    p.lockDevice = lockDevice;
    p.lock_timeout = lock_timeout;
    p.device = device;
    if (!(r = create_link_1(&p, core)))
        return VXI11_CORE_RPCERR;
    if (lidp)
        *lidp = r->lid;
    if (abortPortp)
        *abortPortp = r->abortPort;
    if (maxRecvSizep)
        *maxRecvSizep = r->maxRecvSize;
    return r->error;
}

int
vxi11_device_write(CLIENT *core, long lid, long flags,
                   unsigned long io_timeout, unsigned long lock_timeout,
                   char *data_val, int data_len, unsigned long *sizep)
{
    Device_WriteParms p;
    Device_WriteResp *r;

    p.lid = lid;
    p.io_timeout = io_timeout;
    p.lock_timeout = lock_timeout;
    p.flags = flags;
    p.data.data_val = data_val;
    p.data.data_len = data_len;
    if (!(r = device_write_1(&p, core)))
        return VXI11_CORE_RPCERR;
    if (sizep)
        *sizep = r->size;
    return r->error;
}

int
vxi11_device_read(CLIENT *core, long lid, long flags, 
                  unsigned long io_timeout, unsigned long lock_timeout, 
                  int termChar, int *reasonp, 
                  char *data_val, int *data_lenp, unsigned long requestSize)
{
    Device_ReadParms p;
    Device_ReadResp *r;

    p.lid = lid;
    p.requestSize = requestSize;
    p.io_timeout = io_timeout;
    p.lock_timeout = lock_timeout;
    p.flags = flags;
    p.termChar = termChar;
    if (!(r = device_read_1(&p, core)))
        return VXI11_CORE_RPCERR;
    if (reasonp)
        *reasonp = r->reason;
    if (data_lenp)
        *data_lenp = r->data.data_len;
    if (data_val && r->data.data_val)
       memcpy(data_val, r->data.data_val, r->data.data_len);
    return r->error;
}

int
vxi11_device_readstb(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout,
                     unsigned char *stbp)
{
    Device_GenericParms p;
    Device_ReadStbResp *r;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if (!(r = device_readstb_1(&p, core)))
        return VXI11_CORE_RPCERR;
    if (stbp)
        *stbp = r->stb;
    return r->error;
}

int
vxi11_device_trigger(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout)
{
    Device_GenericParms p;
    Device_Error *r;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if (!(r = device_trigger_1(&p, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_device_clear(CLIENT *core, long lid, long flags, 
                     unsigned long io_timeout, unsigned long lock_timeout)
{
    Device_GenericParms p;
    Device_Error *r;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if (!(r = device_clear_1(&p, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_device_remote(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout)
{
    Device_GenericParms p;
    Device_Error *r;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if (!(r = device_remote_1(&p, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_device_local(CLIENT *core, long lid, long flags,
                   unsigned long io_timeout, unsigned long lock_timeout)
{
    Device_GenericParms p;
    Device_Error *r;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    p.io_timeout = io_timeout;
    if (!(r = device_local_1(&p, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_device_lock(CLIENT *core, long lid, long flags, 
                  unsigned long lock_timeout)
{
    Device_LockParms p;
    Device_Error *r;

    p.lid = lid;
    p.flags = flags;
    p.lock_timeout = lock_timeout;
    if (!(r = device_lock_1(&p, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_device_unlock(CLIENT *core, long lid)
{
    Device_Error *r;

    if (!(r = device_unlock_1(&lid, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_device_enable_srq(CLIENT *core, long lid, int enable, char *handle_val,
                        int handle_len)
{
    Device_EnableSrqParms p;
    Device_Error *r;

    p.lid = lid;
    p.enable = enable;
    p.handle.handle_val = handle_val;
    p.handle.handle_len = handle_len; /* XXX max 40 */
    if (!(r = device_enable_srq_1(&p, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
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

    p.lid = lid;
    p.flags = flags;
    p.io_timeout = io_timeout;
    p.lock_timeout = lock_timeout;
    p.cmd = cmd;
    p.network_order = network_order;
    p.datasize = datasize;
    p.data_in.data_in_val = data_in_val;
    p.data_in.data_in_len = data_in_len;
    if (!(r = device_docmd_1(&p, core)))
        return VXI11_CORE_RPCERR;
    if (data_out_valp)
        *data_out_valp = r->data_out.data_out_val;
    if (data_out_lenp)
        *data_out_lenp = r->data_out.data_out_len;
    return r->error;
}

int
vxi11_destroy_link(CLIENT *core, long lid)
{
    Device_Error *r;

    if (!(r = destroy_link_1(&lid, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_create_intr_chan(CLIENT *core, unsigned long hostAddr,
                       unsigned short hostPort, unsigned long progNum,
                       unsigned long progVers, int progFamily)
{
    Device_RemoteFunc p;
    Device_Error *r;

    p.hostAddr = hostAddr;
    p.hostPort = hostPort;
    p.progNum = progNum;
    p.progVers = progVers;
    p.progFamily = progFamily;
    if (!(r = create_intr_chan_1(&p, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_destroy_intr_chan(CLIENT *core)
{
    Device_Error *r;

    if (!(r = destroy_intr_chan_1(NULL, core)))
        return VXI11_CORE_RPCERR;
    return r->error;
}

int
vxi11_device_abort(CLIENT *abrt, long lid)
{
    Device_Error *r;

    if (!(r = device_abort_1(&lid, abrt)))
        return VXI11_ABRT_RPCERR;
    return r->error;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
