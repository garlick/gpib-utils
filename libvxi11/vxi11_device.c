/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.
  
   Copyright (C) 2001-2011 Jim Garlick <garlick.jim@gmail.com>
  
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
#include <config.h>
#endif
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#if HAVE_STDBOOL_H
#include <stdbool.h> 
#else
typedef enum { false=0, true=1 } bool;
#endif
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <netdb.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/time.h>

#include "vxi11.h"
#include "vxi11_core.h"
#include "vxi11_device.h"

/* Set to 1 to work around old ICS 8064 firmware (see comment below) */
#define ICS8064_OLDFW_WORKAROUND 1


#define VXI11_DFLT_TERMCHAR     0x0a
#define VXI11_DFLT_TERMCHARSET  false
#define VXI11_DFLT_DOLOCKING    false
#define VXI11_DFLT_DOENDW       true

#define VXI11_MAGIC             0x343422aa
#define VXI11_NOLID             (-1)

static bool vxi11_device_debug = false;

struct vxi11_device_struct {
    int             vxi11_magic;
    char            vxi11_devname[MAXHOSTNAMELEN];
    CLIENT         *vxi11_core;
    CLIENT         *vxi11_abrt;
    long            vxi11_lid;
    unsigned short  vxi11_abortPort;
    int             vxi11_termChar;
    bool            vxi11_termCharSet;
    bool            vxi11_doEndw;
    bool            vxi11_doLocking;
    unsigned long   vxi11_lock_timeout;
    unsigned long   vxi11_io_timeout;
    unsigned long   vxi11_maxRecvSize;
    int             vxi11_clientId;
    char            vxi11_errstr[128];
};

struct errtab_struct {
    int num;
    char *desc;
};

static struct errtab_struct  errtab[] = {
    { VXI11_CORE_CREATE, "create core RPC channel" },
    { VXI11_ABRT_CREATE, "create abrt RPC channel" },
    { VXI11_CORE_RPCERR, "error on core RPC channel" },
    { VXI11_ABRT_RPCERR, "error on abrt RPC channel" },
    { VXI11_ERR_SUCCESS, "success" },
    { VXI11_ERR_SYNTAX, "syntax error" },
    { VXI11_ERR_NODEVICE, "no device" },
    { VXI11_ERR_LINKINVAL,"invalid link" },
    { VXI11_ERR_PARAMETER, "parameter error" },
    { VXI11_ERR_NOCHAN, "channel not established" },
    { VXI11_ERR_NOTSUPP, "unsupported operation" },
    { VXI11_ERR_RESOURCES, "insufficent resources" },
    { VXI11_ERR_LOCKED, "device locked" },
    { VXI11_ERR_NOLOCK, "device unlocked" },
    { VXI11_ERR_IOTIMEOUT, "I/O timeout" },
    { VXI11_ERR_IOERROR, "I/O error" },
    { VXI11_ERR_ADDRINVAL, "address invalid" },
    { VXI11_ERR_ABORT, "I/O operation aborted" },
    { VXI11_ERR_CHANEST, "channel already established" },
    { 0, NULL }
};

void 
vxi11_destroy(vxi11dev_t v)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    assert(v->vxi11_lid == VXI11_NOLID);
    assert(v->vxi11_abrt == NULL);
    assert(v->vxi11_core == NULL);
    memset(v, 0, sizeof(struct vxi11_device_struct));
    free(v);
}

vxi11dev_t 
vxi11_create(void)
{
    vxi11dev_t v = malloc(sizeof(struct vxi11_device_struct));

    if (v) {
        v->vxi11_magic        = VXI11_MAGIC;
        v->vxi11_devname[0]   = '\0';
        v->vxi11_core         = NULL;
        v->vxi11_abrt         = NULL;
        v->vxi11_lid          = VXI11_NOLID;
        v->vxi11_abortPort    = 0;
        v->vxi11_termChar     = VXI11_DFLT_TERMCHAR;
        v->vxi11_termCharSet  = VXI11_DFLT_TERMCHARSET;
        v->vxi11_doEndw       = VXI11_DFLT_DOENDW;
        v->vxi11_doLocking    = VXI11_DFLT_DOLOCKING;
        v->vxi11_lock_timeout = 25000; // Default for rpcgen (see libvxi11/vxi11_clnt.c line 62 and 73)
        v->vxi11_io_timeout   = 25000;
        v->vxi11_maxRecvSize  = 0;
        v->vxi11_clientId     = 0;
    }
    return v;
}

static char *
_find_before_colon(char *s, char *buf, int len)
{
    char *p = buf;
    while (--len > 0 && *s != 0 && *s != ':')
        *p++ = *s++;
    *p = '\0';
    return buf;
}

static char *
_find_after_colon(char *s)
{
    char *p = strchr(s, ':');
    return (p ? p + 1 : s);
}

int
vxi11_open(vxi11dev_t v, char *name, bool doAbort)
{
    char tmpbuf[MAXHOSTNAMELEN];
    char *device, *hostname;
    int res;

    assert(v->vxi11_magic == VXI11_MAGIC);
    strncpy(v->vxi11_devname, name, MAXHOSTNAMELEN);
    v->vxi11_devname[MAXHOSTNAMELEN - 1] = '\0';

    hostname = _find_before_colon(v->vxi11_devname, tmpbuf, sizeof(tmpbuf));
    if ((res = vxi11_open_core_channel(hostname, &v->vxi11_core)) != 0)
        goto err;

    device = _find_after_colon(v->vxi11_devname);
    if ((res = vxi11_create_link(v->vxi11_core, v->vxi11_clientId, 
                             v->vxi11_doLocking, v->vxi11_lock_timeout, 
                             device, &v->vxi11_lid, &v->vxi11_abortPort,
                             &v->vxi11_maxRecvSize)) != 0)
        goto err;
    if (doAbort)  {
        if ((res = vxi11_open_abrt_channel(v->vxi11_core, v->vxi11_abortPort, 
                &v->vxi11_abrt)) != 0)
            goto err;
    }
    return res;
err:
    vxi11_close(v);
    return res;
}

void
vxi11_close(vxi11dev_t v)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_abrt)
        vxi11_close_abrt_channel(v->vxi11_abrt);
    v->vxi11_abrt = NULL;
    if (v->vxi11_lid != VXI11_NOLID)
        (void)vxi11_destroy_link(v->vxi11_core, v->vxi11_lid);
    v->vxi11_lid = VXI11_NOLID;
    if (v->vxi11_core)
        vxi11_close_core_channel(v->vxi11_core);
    v->vxi11_core = NULL;
}


static unsigned long
_timersubms(struct timeval *a, struct timeval *b)
{
    struct timeval t;

    t.tv_sec =  a->tv_sec  - b->tv_sec;
    t.tv_usec = a->tv_usec - b->tv_usec;
    if (t.tv_usec < 0) {
        t.tv_sec--;
        t.tv_usec += 1000000;    
    }
    return (t.tv_usec / 1000 + t.tv_sec * 1000);
}

/* Execute multiple write RPC's of maxRecvSize or less.  
 * If doLocking, take one lock covering multiple write RPC's.
 * Decrease io_timeout each time through the loop.
 * If doEndw, set ENDW flag on the last chunk.
 */
int 
vxi11_write(vxi11dev_t v, char *buf, int len)
{
    long flags = 0;
    unsigned long size, try, tmout;
    int res = 0, lres;
    struct timeval t1, t2;

    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;

    if (v->vxi11_doLocking && (lres = vxi11_lock(v)) != 0)
        return lres;
    tmout = v->vxi11_io_timeout; 
    while (res == 0 && len > 0) {
        if (len > v->vxi11_maxRecvSize) {
            try = v->vxi11_maxRecvSize;
        } else {
            try = len;
            if (v->vxi11_doEndw)
                flags |= VXI11_FLAG_ENDW;
        }
        gettimeofday(&t1, NULL);
        res = vxi11_device_write(v->vxi11_core, v->vxi11_lid, flags,
                                 tmout, 0, buf, try, &size);
        gettimeofday(&t2, NULL);
        if (res == 0) {
#if ICS8064_OLDFW_WORKAROUND
            if (size == 0)   /* ICS 8064 Rev X0.00 Ver 08.01.22 */
                size = try;  /*  size = 0 on success, which violates B.6.21 */
#endif
            buf -= size;
            len -= size;
            tmout -= _timersubms(&t2, &t1);
            if (len > 0 && tmout <= 0)
                res = VXI11_ERR_IOTIMEOUT;
        }
    }
    if (v->vxi11_doLocking && (lres = vxi11_unlock(v)) != 0)
        if (res == 0)
            return lres;

    return res;
}

int 
vxi11_writestr(vxi11dev_t v, char *str)
{
    return vxi11_write(v, str, strlen(str));
}

/* Execute multiple read RPC's.
 * If doLocking, take one lock covering multiple read RPC's.
 * Decrease io_timeout each time through the loop.
 */
int 
vxi11_read(vxi11dev_t v, char *buf, int len, int *numreadp)
{
    int lres, res = 0;
    struct timeval t1, t2;
    unsigned long tmout;
    char *data;
    int try;
    long flags = 0;
    int reason = 0;
    int count = 0;

    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    if (v->vxi11_termCharSet)
        flags |= VXI11_FLAG_TERMCHRSET;

    if (v->vxi11_doLocking && (lres = vxi11_lock(v)) != 0)
            return lres;
    tmout = v->vxi11_io_timeout; 
    while (res == 0 && len > 0 && reason == 0) {
        gettimeofday(&t1, NULL);
        res = vxi11_device_read(v->vxi11_core, v->vxi11_lid, flags, 
                                tmout, 0, v->vxi11_termChar, &reason, 
                                buf, &try, len);
        gettimeofday(&t2, NULL);
        if (res == 0) {
            count += try;
            len -= try;
            buf += try;
            tmout -= _timersubms(&t2, &t1);
            if (len > 0 && tmout <= 0)
                res = VXI11_ERR_IOTIMEOUT;
        }
    }
    if (v->vxi11_doLocking && (lres = vxi11_unlock(v)) != 0)
        if (res == 0)
            return lres;

    if (numreadp)
        *numreadp = count;
    return res;
}

int 
vxi11_readstr(vxi11dev_t v, char *str, int len)
{
    int res, count;

    res = vxi11_read(v, str, len - 1, &count);
    if (res == 0) {
        assert(count < len);
        str[count] = '\0';
    }
    return res;
}

int 
vxi11_readstb(vxi11dev_t v, unsigned char *stbp)
{
    long flags = 0;

    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    if (v->vxi11_doLocking)
        flags |= VXI11_FLAG_WAITLOCK;
    return vxi11_device_readstb(v->vxi11_core, v->vxi11_lid, flags,
                                v->vxi11_io_timeout, v->vxi11_lock_timeout,
                                stbp);
}

int 
vxi11_trigger(vxi11dev_t v)
{
    long flags = 0;

    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    if (v->vxi11_doLocking)
        flags |= VXI11_FLAG_WAITLOCK;
    return vxi11_device_trigger(v->vxi11_core, v->vxi11_lid, flags,
                                v->vxi11_io_timeout, v->vxi11_lock_timeout);
}

int 
vxi11_clear(vxi11dev_t v)
{
    long flags = 0;

    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    if (v->vxi11_doLocking)
        flags |= VXI11_FLAG_WAITLOCK;
    return vxi11_device_clear(v->vxi11_core, v->vxi11_lid, flags,
                              v->vxi11_io_timeout, v->vxi11_lock_timeout);
}

int 
vxi11_remote(vxi11dev_t v)
{
    long flags = 0;

    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    if (v->vxi11_doLocking)
        flags |= VXI11_FLAG_WAITLOCK;
    return vxi11_device_remote(v->vxi11_core, v->vxi11_lid, flags,
                              v->vxi11_io_timeout, v->vxi11_lock_timeout);
}

int 
vxi11_local(vxi11dev_t v)
{
    long flags = 0;

    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    if (v->vxi11_doLocking)
        flags |= VXI11_FLAG_WAITLOCK;
    return vxi11_device_local(v->vxi11_core, v->vxi11_lid, flags,
                              v->vxi11_io_timeout, v->vxi11_lock_timeout);
}

int 
vxi11_lock(vxi11dev_t v)
{
    long flags = 0;

    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    if (v->vxi11_doLocking)
        flags |= VXI11_FLAG_WAITLOCK;
    return vxi11_device_lock(v->vxi11_core, v->vxi11_lid, 
                             flags, v->vxi11_lock_timeout);
}

int 
vxi11_unlock(vxi11dev_t v)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_core == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    return vxi11_device_unlock(v->vxi11_core, v->vxi11_lid);
}

int 
vxi11_abort(vxi11dev_t v)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    if (v->vxi11_abrt == NULL)
        return VXI11_ERR_NOCHAN;
    if (v->vxi11_lid == VXI11_NOLID)
        return VXI11_ERR_LINKINVAL;
    return vxi11_device_abort(v->vxi11_abrt, v->vxi11_lid);
}

void
vxi11_set_iotimeout(vxi11dev_t v, unsigned long timeout)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    v->vxi11_io_timeout = timeout;
    if (v->vxi11_core != NULL) {
        struct timeval io_timeout;
        timerclear(&io_timeout);
        io_timeout.tv_sec = timeout / 1000;
        io_timeout.tv_usec = (timeout % 1000) * 1000;
        clnt_control(v->vxi11_core, CLSET_TIMEOUT, (caddr_t)&io_timeout);
    }
}

void
vxi11_set_lockpolicy(vxi11dev_t v, bool doLocking, unsigned long timeout)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    v->vxi11_doLocking = doLocking;
    v->vxi11_lock_timeout = timeout;
}

void
vxi11_set_termchar(vxi11dev_t v, int termChar)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    v->vxi11_termChar = termChar;
}

void
vxi11_set_termcharset(vxi11dev_t v, bool termCharSet)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    v->vxi11_termCharSet = termCharSet;
}

void
vxi11_set_endw(vxi11dev_t v, bool doEndw)
{
    assert(v->vxi11_magic == VXI11_MAGIC);
    v->vxi11_doEndw = doEndw;
}

void vxi11_set_device_debug(bool doDebug)
{
    vxi11_set_core_debug(doDebug);
    vxi11_device_debug = 1;
}

static char *
_lookup_err(int err)
{
    int i;
    char *desc = NULL;

    for (i = 0; errtab[i].desc != NULL; i++) {
        if (errtab[i].num == err) {
            desc = errtab[i].desc;
            break;
        }
    }
    return desc ? desc : "unknown error";
}    

void 
vxi11_perror(vxi11dev_t v, int err, char *str)
{
    char *errstr = vxi11_strerror (v, err);

    fprintf(stderr, "%s (%s): %s\n", str, v->vxi11_devname, errstr);
}

char * 
vxi11_strerror(vxi11dev_t v, int err)
{
    char *desc;

    switch (err) {
        case VXI11_CORE_CREATE:
            desc = clnt_spcreateerror("core");
            break;
        case VXI11_ABRT_CREATE:
            desc = clnt_spcreateerror("abrt");
            break;
        case VXI11_CORE_RPCERR:
            desc = clnt_sperror(v->vxi11_core, "core");
            break;
        case VXI11_ABRT_RPCERR:
            desc = clnt_sperror(v->vxi11_abrt, "abrt");
            break;
        default:
            desc = _lookup_err(err);
            break;
    }
    snprintf (v->vxi11_errstr, sizeof (v->vxi11_errstr), "%s", desc);
    /* N.B. clnt_sp* errors are \n terminated */
    if (v->vxi11_errstr[strlen (v->vxi11_errstr) - 1] == '\n')
        v->vxi11_errstr[strlen (v->vxi11_errstr) - 1] = '\0';

    return v->vxi11_errstr;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
