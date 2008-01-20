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
struct vxi_handle_struct {
    int             magic;
    CLIENT         *vxi_core_cli;   /* core channel */
    CLIENT         *vxi_abrt_cli;   /* abort channel */
    Device_Link     vxi_lid;        /* device link */
    unsigned long   vxi_writemax;   /* max write size for device */
};

int
vxi11_device_read(vxi_handle_t vp, char *buf, int len, 
                  int flags, unsigned long locktmout, unsigned long iotmout, 
                  int termchar, long *errp, int *reasonp)
{
    Device_ReadParms p;
    Device_ReadResp *r;

    assert(vp->magic == VXI_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL) {
        if (errp)
            *errp = VXI_ERR_NOCHAN;
        return -1;
    }
    p.lid = vp->vxi_lid;
    p.requestSize = len;
    p.io_timeout = iotmout;
    p.lock_timeout = locktmout;
    p.flags = flags;
    p.termChar = termchar;
    r = device_read_1(&p, vp->vxi_core_cli);
    if (r == NULL) {
        if (errp)
            *errp = VXI_ERR_RPCERR;
        return -1;
    } else if (r->error != 0) {
        if (errp)
            *errp = r->error;
        return -1;
    }
    memcpy(buf, r->data.data_val, r->data.data_len);
    if (reasonp)
        *reasonp = r->reason;
    return r->data.data_len;
}

void 
vxi11_device_write(vxi_handle_t vp, char *buf, int len, int flags, 
                   unsigned long locktmout, unsigned long iotmout, long *errp)
{
    Device_WriteParms p;
    Device_WriteResp *r;

    assert(vp->magic == VXI_HANDLE_MAGIC);
    if (vp->vxi_core_cli == NULL) {
        if (errp)
            *errp = VXI_ERR_NOCHAN;
        return -1;
    }
    if (len > vxi->vxi_maxwrite) {
        if (errp)
            *errp = VXI_ERR_TOOBIG;
        return -1;
    }
    p.lid = vp->vxi_lid;
    p.io_timeout = iotmout;
    p.lock_timeout = locktmout;
    p.flags = flags;
    p.data.data_val = buf;
    p.data.data_len = len;
    r = device_write_1(&p, vp->vxi_core_cli);
    if (r == NULL) {
        if (errp)
            *errp = VXI_ERR_RPCERR;
        return -1;
    } else if (r->error != 0) {
        if (errp)
            *errp = r->error;
        return -1;
    }
    return r->size;
}

int
vxi11_device_readstb(vxi_handle_t gd, unsigned long locktmout, int flags,
                     unsigned long iotmout, unsigned char *statusp, long *errp)
{
    Device_GenericParms p;
    Device_ReadStbResp *r;

    assert(vp->magic == VXI_HANDLE_MAGIC);
    assert(statusp != NULL);
    if (vp->vxi_core_cli == NULL) {
        if (errp)
            *errp = VXI_ERR_NOCHAN;
        return -1;
    }
    p.lid = vp->vxi_lid;
    p.flags = flags;
    p.lock_timeout = locktmout
    p.io_timeout = iotmout
    r = device_readstb_1(&p, vp->vxi_core_cli);
    if (r == NULL) {
        if (errp)
            *errp = VXI_ERR_RPCERR;
        return -1;
    }
    if (r->error) {
        if (errp)
            *errp = r->error;
        return -1;
    }
    if (status)
        *status = r->stb;
    return 0;
}

void
vxi11_device_trigger(vxi_handle_t vp)
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

void
vxi11_close(vxi_handle_t vp)
{
    Device_Error *r;

    assert(vp->magic == VXI_HANDLE_MAGIC);
    if (vp->vxi_lid != -1)
        r = destroy_link_1(&gd->vxi_lid, gd->vxi_cli); /* ignore errors */
    if (vp->vxi_abrt_cli)
        clnt_destroy(vp->vxi_abrt_cli);
    if (vp->vxi_core_cli)
        clnt_destroy(vp->vxi_core_cli);
    memset(vp, 0, sizeof(struct vxi_handle_struct));
    free(vp);
}

/* helper for vxi11_open */
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

vxi_handle_t
vxi11_open(char *host, char *device, int lockdevice, 
           unsigned long locktmout, unsigned long *maxrecvsizep, long *errp);
{
    vxi_handle_t vp;=
    Create_LinkParms p;
    Create_LinkResp *r;
    struct sockaddr_in addr;
    int sock;

    vp = malloc(sizeof(vxi_handle_struct));
    if (!vp) {
        if (errp)
            *errp = VXI11_ERR_RESOURCES;
        return NULL;
    }
    vp->vxi_magic = VXI_HANDLE_MAGIC;

    /* open core connection */
    vp->vxi_core_cli = clnt_create(host, DEVICE_CORE, DEVICE_CORE_VERSION, 
                                   "tcp");
    if (vp->vxi_core_cli == NULL) {
        vxi11_close(vp, errp);
        if (errp)
            *errp = VXI_ERR_RPCERR;
        return NULL;
    }

    /* establish link to instrument */
    p.clientId = 0; /* not used */
    p.lockDevice = lockdevice;
    p.lock_timeout = locktmout;
    p.device = device;
    r = create_link_1(&p, new->vxi_cli);
    if (r == NULL) {
        vxi11_close(vp, errp);
        if (errp)
            *errp = VXI11_ERR_RPCERR;
        return NULL;
    }
    vp->vxi_lid = r->lid;
    vp->vxi_writemax = r->maxRecvSize;

    /* open abrt connection */
    if (_get_sockaddr(host, r->abortPort, &addr) == -1) {
        vxi11_close(vp, errp);
        if (errp)
            *errp = VXI11_ERR_ABRTINFO;
        return NULL;
    }
    sock = RPC_ANYSOCK;
    vp->vxi_abrt_cli = clnttcp_create(&addr, DEVICE_ASYNC, 
				       DEVICE_ASYNC_VERSION, &sock, 0, 0);
    if (vp->vxi_abrt_cli == NULL) {
        vxi11_close(vp, errp);
        if (errp)
            *errp = VXI_ERR_RPCERR;
        return NULL;
    }

    if (maxrecvsizep)
        *maxrecvsizep = r->maxRecvSize;
    return vp;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
