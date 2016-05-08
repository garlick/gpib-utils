/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2007 Jim Garlick <garlick.jim@gmail.com>

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
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <string.h>

#include <assert.h>

#include "ics8000.h"
#include "ics.h"

#define ICS_MAGIC 0x23434666
struct ics_struct {
    int         ics_magic;
    CLIENT     *ics_clnt;
    char        ics_errstr[128];
};

typedef struct {
    int     num;
    char   *str;
} strtab_t;

static strtab_t errtab[] = {
    { ICS_SUCCESS,              "success" },
    { ICS_ERROR_SYNTAX,         "syntax error" },
    { ICS_ERROR_PARAMETER,      "parameter error" },
    { ICS_ERROR_UNSUPPORTED,    "unsupported function" },
    { 0,    NULL },
};

static char *
_mkstr(char *data, u_int len)
{
    char *new = malloc(len + 1);
    assert (new != NULL);
    memcpy(new, data, len);
    new[len] = '\0';
    return new;
}

static unsigned int
_reverse(unsigned int i)
{ 
    assert(sizeof(unsigned int) == 4);
    return (((i & 0xff000000) >> 24) | ((i & 0x00ff0000) >>  8) | 
            ((i & 0x0000ff00) <<  8) | ((i & 0x000000ff) << 24));
}

static char *
_ip2str(unsigned int ip)
{
    struct in_addr in;
    char *cpy;

    in.s_addr = _reverse(htonl(ip));
    cpy = strdup(inet_ntoa(in));
    assert (cpy != NULL);
    return cpy;
}

static int
_str2ip(char *str, unsigned int *ipp)
{
    struct in_addr in;

    if (inet_aton(str, &in) == 0)
        return -1;
    *ipp = ntohl(_reverse(in.s_addr));
    return 0;
}

int
ics_get_interface_name(ics_t ics, char **strp)
{
    Int_Name_Parms p;
    Int_Name_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    p.name.name_len = 0;
    r = interface_name_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *strp = _mkstr(r->name.name_val, r->name.name_len);
    return 0;
}

int
ics_set_interface_name(ics_t ics, char *str)
{
    Int_Name_Parms p;
    Int_Name_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    p.name.name_val = str;
    p.name.name_len = strlen(str);
    r = interface_name_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_get_comm_timeout(ics_t ics, unsigned int *timeoutp)
{
    Comm_Timeout_Parms p;
    Comm_Timeout_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = comm_timeout_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *timeoutp = r->timeout;
    return 0;
}

int
ics_set_comm_timeout(ics_t ics, unsigned int timeout)
{
    Comm_Timeout_Parms p;
    Comm_Timeout_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    p.timeout = timeout;
    r = comm_timeout_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_get_static_ip_mode(ics_t ics, int *flagp)
{
    Static_IP_Parms p;
    Static_IP_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = static_ip_mode_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *flagp = r->mode;
    return 0;
}

int
ics_set_static_ip_mode(ics_t ics, int flag)
{
    Static_IP_Parms p;
    Static_IP_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    p.mode = flag;
    r = static_ip_mode_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_get_ip_number(ics_t ics, char **ipstrp)
{
    IP_Number_Parms p;
    IP_Number_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = ip_number_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *ipstrp = _ip2str(r->ip);
    return 0;
}

int
ics_set_ip_number(ics_t ics, char *ipstr)
{
    IP_Number_Parms p;
    IP_Number_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    if (_str2ip(ipstr, &p.ip) < 0)
        return ICS_ERROR_PARAMETER;
    r = ip_number_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_get_netmask(ics_t ics, char **ipstrp)
{
    Netmask_Parms p;
    Netmask_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = netmask_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *ipstrp = _ip2str(r->ip);
    return 0;
}

int
ics_set_netmask(ics_t ics, char *ipstr)
{
    Netmask_Parms p;
    Netmask_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    if (_str2ip(ipstr, &p.ip) < 0)
        return ICS_ERROR_PARAMETER;
    r = netmask_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_get_gateway(ics_t ics, char **ipstrp)
{
    Gateway_Parms p;
    Gateway_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = gateway_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *ipstrp = _ip2str(r->ip);
    return 0;
}

int
ics_set_gateway(ics_t ics, char *ipstr)
{
    Gateway_Parms p;
    Gateway_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    if (_str2ip(ipstr, &p.ip) < 0)
        return ICS_ERROR_PARAMETER;
    r = gateway_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_get_keepalive(ics_t ics, unsigned int *timep)
{
    Keepalive_Parms p;
    Keepalive_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = keepalive_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *timep = r->time;
    return 0;
}

int
ics_set_keepalive(ics_t ics, unsigned int time)
{
    Keepalive_Parms p;
    Keepalive_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    p.time = time;
    r = keepalive_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_get_gpib_address(ics_t ics, unsigned int *addrp)
{
    Gpib_Addr_Parms p;
    Gpib_Addr_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = gpib_address_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *addrp = r->address;
    return 0;
}

int
ics_set_gpib_address(ics_t ics, unsigned int addr)
{
    Gpib_Addr_Parms p;
    Gpib_Addr_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    p.address = addr;
    r = gpib_address_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int     
ics_get_system_controller(ics_t ics, int *flagp)
{
    Sys_Control_Parms p;
    Sys_Control_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = system_controller_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *flagp = r->controller;
    return 0;
}

int     
ics_set_system_controller(ics_t ics, int flag)
{
    Sys_Control_Parms p;
    Sys_Control_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    p.controller = flag;
    r = system_controller_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int     
ics_get_ren_mode(ics_t ics, int *flagp)
{
    Ren_Parms p;
    Ren_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_READ;
    r = ren_mode_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *flagp = r->ren;
    return 0;
}

int     
ics_set_ren_mode(ics_t ics, int flag)
{
    Ren_Parms p;
    Ren_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    p.action = ICS_WRITE;
    p.ren = flag;
    r = ren_mode_1(&p, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_reload_config(ics_t ics)
{
    Reload_Config_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = reload_config_1(NULL, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_reload_factory(ics_t ics)
{
    Reload_Factory_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = reload_factory_1(NULL, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_commit_config(ics_t ics)
{
    Commit_Config_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = commit_config_1(NULL, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_reboot(ics_t ics)
{
    Reboot_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = reboot_1(NULL, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    return 0;
}

int
ics_idn_string(ics_t ics, char **strp)
{
    Idn_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = idn_string_1(NULL, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *strp = _mkstr(r->idn.idn_val, r->idn.idn_len);
    return 0;
}

int
ics_error_logger(ics_t ics, unsigned int **errp, int *countp)
{
    Error_Log_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = error_logger_1(NULL, ics->ics_clnt);
    if (r == NULL)
        return ICS_ERROR_CLNT;
    if (r->error)
        return r->error;
    *errp = malloc(r->count * sizeof(unsigned int));
    assert (*errp != NULL);
    memcpy(*errp, r->errors, r->count * sizeof(unsigned int));
    *countp = r->count;
    return 0;
}

void
ics_fini(ics_t ics)
{
    assert(ics->ics_magic == ICS_MAGIC);
    if (ics->ics_clnt)
        clnt_destroy(ics->ics_clnt);
    ics->ics_magic = 0;
    free(ics);
}

int
ics_init (char *host, ics_t *icsp)
{
    ics_t new;
    char *p;

    new = malloc(sizeof(struct ics_struct));
    assert (new != NULL);
    new->ics_magic = ICS_MAGIC;
    *icsp = new;

    new->ics_clnt = clnt_create(host, ICSCONFIG, ICSCONFIG_VERSION, "tcp");
    if (new->ics_clnt == NULL)
        return ICS_ERROR_CREATE;
    return 0;
}

static char *
_lookup_err(int err)
{
    int i;
    char *desc = NULL;

    for (i = 0; errtab[i].str != NULL; i++) {
        if (errtab[i].num == err) {
            desc = errtab[i].str;
            break;
        }
    }
    return desc ? desc : "unknown error";
}

char *
ics_strerror(ics_t ics, int err)
{
    char *desc;

    switch (err) {
        case ICS_ERROR_CREATE:
            desc = clnt_spcreateerror("");
            while (*desc != ':')
                desc++;
            desc++;
            while (isspace (*desc))
                desc++;
            break;
        case ICS_ERROR_CLNT:
            desc = clnt_sperror(ics->ics_clnt, "");
            while (*desc != ':')
                desc++;
            desc++;
            while (isspace (*desc))
                desc++;
            break;
        default:
            desc = _lookup_err(err);
            break;
    }
    snprintf (ics->ics_errstr, sizeof (ics->ics_errstr), "%s", desc);
    /* N.B. clnt_sp* errors are \n terminated */
    if (ics->ics_errstr[strlen (ics->ics_errstr) - 1] == '\n')
        ics->ics_errstr[strlen (ics->ics_errstr) - 1] = '\0';

    return ics->ics_errstr;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
