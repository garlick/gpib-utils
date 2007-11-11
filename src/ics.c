/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2007 Jim Garlick <garlick@speakeasy.net>

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

#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <arpa/inet.h>

#include <assert.h>

#include "ics8000.h"
#include "ics.h"
#include "util.h"
#include "errstr.h"

extern char *prog;

#define ICS_MAGIC 0x23434666
struct ics_struct {
    int         ics_magic;
    CLIENT     *ics_clnt;
};

static errstr_t errtab[] = {
    { 0,    "success" },
    { 1,    "syntax error" },
    { 5,    "parameter error" },
    { 0,    NULL },
};

static char *
_mkstr(char *data, u_int len)
{
    char *new = xmalloc(len + 1);

    memcpy(new, data, len);
    new[len] = '\0';
    return new;
}

static unsigned int
_reverse(unsigned int i)
{ 
    return (((i & 0xff000000) >> 24) | ((i & 0x00ff0000) >>  8) | 
            ((i & 0x0000ff00) <<  8) | ((i & 0x000000ff) << 24));
}

static char *
_ip2str(unsigned int ip)
{
    struct in_addr in;

    in.s_addr = _reverse(htonl(ip));
    return xstrdup(inet_ntoa(in));
}

static int
_str2ip(char *str, unsigned int *ipp)
{
    struct in_addr in;

    if (inet_aton(str, &in) == 0) {
        fprintf(stderr, "%s: invalid ip number\n", prog);
        return 1;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: interface_name: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: interface_name: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: comm_timeout: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: comm_timeout: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: static_ip_mode: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: static_ip_mode: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: ip_number: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (_str2ip(ipstr, &p.ip))
        return 1;
    r = ip_number_1(&p, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: ip_number: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: netmask: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (_str2ip(ipstr, &p.ip))
        return 1;
    r = netmask_1(&p, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: netmask: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: gateway: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (_str2ip(ipstr, &p.ip))
        return 1;
    r = gateway_1(&p, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: gateway: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: keepalive: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: keepalive: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: gpib_address: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: gpib_address: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: system_controller: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: system_controller: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: ren_mode: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
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
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: ren_mode: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
    return 0;
}

int
ics_reload_config(ics_t ics)
{
    Reload_Config_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = reload_config_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: reload_config: %s\n", 
                prog, finderr(errtab, r->error));
        return r->error;
    }
    return 0;
}

int
ics_reload_factory(ics_t ics)
{
    Reload_Factory_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = reload_factory_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: reload_factory: %s\n", 
                prog, finderr(errtab, r->error));
        return r->error;
    }
    return 0;
}

int
ics_commit_config(ics_t ics)
{
    Commit_Config_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = commit_config_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: commit_config: %s\n", 
                prog, finderr(errtab, r->error));
        return r->error;
    }
    return 0;
}

int
ics_reboot(ics_t ics)
{
    Reboot_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = reboot_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: reboot: %s\n", 
                prog, finderr(errtab, r->error));
        return r->error;
    }
    return 0;
}

int
ics_idn_string(ics_t ics, char **strp)
{
    Idn_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = idn_string_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: idn_string: %s\n", 
                prog, finderr(errtab, r->error));
        return r->error;
    }
    *strp = _mkstr(r->idn.idn_val, r->idn.idn_len);
    return 0;
}

int
ics_error_logger(ics_t ics, unsigned int **errp, int *countp)
{
    Error_Log_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = error_logger_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error) {
        fprintf(stderr, "%s: error_logger: %s\n", prog, 
                finderr(errtab, r->error));
        return r->error;
    }
    *errp = xmalloc(r->count * sizeof(unsigned int));
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

ics_t
ics_init (char *host)
{
    ics_t new;

    new = xmalloc(sizeof(struct ics_struct));
    new->ics_magic = ICS_MAGIC;

    new->ics_clnt = clnt_create(host, ICSCONFIG, ICSCONFIG_VERSION, "tcp");
    if (new->ics_clnt == NULL) {
        clnt_pcreateerror(prog);
        ics_fini(new);
        new = NULL;
    }
    return new;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
