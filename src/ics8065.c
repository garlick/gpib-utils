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

#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ics.h"
#include "util.h"
#include "gpib.h"

#define INSTRUMENT "ics8065"

#define OPTIONS "a:cfCriejJ:tT:mM:sS:zZ:kK:gG:vV:qQ:wW:"
static struct option longopts[] = {
        {"address",             required_argument,  0, 'a'},
        {"get-interface-name",  no_argument,        0, 'j'},
        {"set-interface-name",  required_argument,  0, 'J'},
        {"get-comm-timeout",    no_argument,        0, 't'},
        {"set-comm-timeout",    required_argument,  0, 'T'},
        {"get-static-ip-mode",  no_argument,        0, 's'},
        {"set-static-ip-mode",  required_argument,  0, 'S'},
        {"get-ip-number",       no_argument,        0, 'z'},
        {"set-ip-number",       required_argument,  0, 'Z'},
        {"get-netmask",         no_argument,        0, 'k'},
        {"set-netmask",         required_argument,  0, 'K'},
        {"get-gateway",         no_argument,        0, 'g'},
        {"set-gateway",         required_argument,  0, 'G'},
        {"get-keepalive",       no_argument,        0, 'v'},
        {"set-keepalive",       required_argument,  0, 'V'},
        {"get-gpib-address",    no_argument,        0, 'q'},
        {"set-gpib-address",    required_argument,  0, 'Q'},
        {"get-system-controller",no_argument,       0, 'w'},
        {"set-system-controller",required_argument, 0, 'W'},
        {"set-ren-mode",        required_argument,  0, 'M'},
        {"get-ren-mode",        no_argument,        0, 'm'},
        {"reload-config",       no_argument,        0, 'C'},
        {"reload-factory",      no_argument,        0, 'f'},
        {"commit-config",       no_argument,        0, 'c'},
        {"reboot",              no_argument,        0, 'r'},
        {"get-idn-string",      no_argument,        0, 'i'},
        {"get-errlog",          no_argument,        0, 'e'},
        {0, 0, 0, 0}
};

static strtab_t errtab[] = ICS_ERRLOG;
char *prog;

void
usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
  "Usage: %s [--options]\n"
  "  -a,--address              instrument address [%s]\n"
  "  -j,--get-interface-name   get VXI-11 logical name (e.g. gpib0)\n"
  "  -J,--set-interface-name   set VXI-11 logical name (e.g. gpib0)\n"
  "  -t,--get-comm-timeout     get TCP timeout (in seconds)\n"
  "  -T,--set-comm-timeout     set TCP timeout (in seconds)\n"
  "  -s,--get-static-ip-mode   get static IP mode (0=disabled, 1=enabled)\n"
  "  -S,--set-static-ip-mode   set static IP mode (0=disabled, 1=enabled)\n"
  "  -z,--get-ip-number        get IP number (a.b.c.d)\n"
  "  -Z,--set-ip-number        set IP number (a.b.c.d)\n"
  "  -k,--get-netmask          get netmask (a.b.c.d)\n"
  "  -K,--set-netmask          set netmask (a.b.c.d)\n"
  "  -g,--get-gateway          get gateway (a.b.c.d)\n"
  "  -G,--set-gateway          set gateway (a.b.c.d)\n"
  "  -v,--get-keepalive        get keepalive time (in seconds)\n"
  "  -V,--set-keepalive        set keepalive time (in seconds)\n"
  "  -q,--get-gpib-address     get gpib address\n"
  "  -Q,--set-gpib-address     set gpib address\n"
"  -w,--get-system-controller get sys controller mode (0=disabled, 1=enabled)\n"
"  -W,--set-system-controller set sys controller mode (0=disabled, 1=enabled)\n"
  "  -m,--get-ren-mode         get REN active at boot (0=false, 1=true)\n"
  "  -M,--set-ren-mode         set REN active at boot (0=false, 1=true)\n"
  "  -C,--reload-config        force reload of default config\n"
  "  -f,--reload-factory       reload factory config settings\n"
  "  -c,--commit-config        commit (write) current config\n"
  "  -r,--reboot               reboot\n"
  "  -i,--get-idn-string       get idn string\n"
  "  -e,--get-errlog           get error log (side effect: log is cleared)\n"
                       , prog, addr ? addr : "no default");
        exit(1);

}

int
main(int argc, char *argv[])
{
    ics_t ics;
    int c;
    char *addr = NULL;
    int get_interface_name = 0;
    char *set_interface_name = NULL;
    int get_comm_timeout = 0;
    int set_comm_timeout = -1;
    int get_static_ip_mode = 0;
    int set_static_ip_mode = -1;
    int get_ip_number = 0;
    char *set_ip_number = NULL;
    int get_netmask = 0;
    char *set_netmask = NULL;
    int get_gateway = 0;
    char *set_gateway = NULL;
    int get_keepalive = 0;
    int set_keepalive = -1;
    int get_gpib_address = 0;
    int set_gpib_address = -1;
    int get_system_controller = 0;
    int set_system_controller = -1;
    int get_ren_mode = 0;
    int set_ren_mode = -1;
    int reload_config = 0;
    int reload_factory = 0;
    int commit_config = 0;
    int reboot = 0;
    int get_idn = 0;
    int get_errlog = 0;
    int todo = 0;

    prog = basename(argv[0]);

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'a' :  /* --address */
                addr = optarg;
                break;
            case 'j' :  /* --get-interface-name */
                get_interface_name = 1;
                todo++;
                break;
            case 'J' :  /* --set-interface-name */
                set_interface_name = optarg;
                todo++;
                break;
            case 't' :  /* --get-comm-timeout */
                get_comm_timeout = 1;
                todo++;
                break;
            case 'T' :  /* --set-comm-timeout */
                set_comm_timeout = strtoul(optarg, NULL, 0);
                todo++;
                break;
            case 's' :  /* --get-static-ip-mode */
                get_static_ip_mode = 1;
                todo++;
                break;
            case 'S' :  /* --set-static-ip-mode */
                set_static_ip_mode = strtoul(optarg, NULL, 0);
                todo++;
                break;
            case 'z' :  /* --get-ip-number */
                get_ip_number = 1;
                todo++;
                break;
            case 'Z' :  /* --set-ip-number */
                set_ip_number = optarg;
                todo++;
                break;
            case 'k' :  /* --get-netmask */
                get_netmask = 1;
                todo++;
                break;
            case 'K' :  /* --set-netmask */
                set_netmask = optarg;
                todo++;
                break;
            case 'g' :  /* --get-gateway */
                get_gateway = 1;
                todo++;
                break;
            case 'G' :  /* --set-gateway */
                set_gateway = optarg;
                todo++;
                break;
            case 'v' :  /* --get-keepalive */
                get_keepalive = 1;
                todo++;
                break;
            case 'V' :  /* --set-keepalive */
                set_keepalive = strtoul(optarg, NULL, 0);
                todo++;
                break;
            case 'q' :  /* --get-gpib-address */
                get_gpib_address = 1;
                todo++;
                break;
            case 'Q' :  /* --set-gpib-address */
                set_gpib_address = strtoul(optarg, NULL, 0);
                todo++;
                break;
            case 'w' :  /* --get-system-controller */
                get_system_controller = 1;
                todo++;
                break;
            case 'W' :  /* --set-system-controller */
                set_system_controller = strtoul(optarg, NULL, 0);
                todo++;
                break;
            case 'm' :  /* --get-ren-mode */
                get_ren_mode = 1;
                todo++;
                break;
            case 'M' :  /* --set-ren-mode */
                set_ren_mode = strtoul(optarg, NULL, 0);
                todo++;
                break;
            case 'C':   /* --reload-config */
                reload_config = 1;
                todo++;
                break;
            case 'f':   /* --reload-factory */
                reload_factory = 1;
                todo++;
                break;
            case 'c':   /* --commit-config */
                commit_config = 1;
                todo++;
                break;
            case 'r' :  /* --reboot */
                reboot = 1;
                todo++;
                break;
            case 'i' :  /* --idn */
                get_idn = 1;
                todo++;
                break;
            case 'e' :  /* --errlog */
                get_errlog = 1;
                todo++;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc || !todo)
        usage();

    if (!addr)
        addr = gpib_default_addr(INSTRUMENT);
    if (!addr) {
        fprintf(stderr, "%s: no default address for %s, use --address\n",
                prog, INSTRUMENT);
        exit(1);
    }
    ics = ics_init(addr);
    if (!ics)
        exit(1);

    /* idn_string */
    if (get_idn) {
        char *tmpstr;

        if (ics_idn_string(ics, &tmpstr) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
        free(tmpstr);
    }

    /* interface_name */
    if (get_interface_name) {
        char *tmpstr;

        if (ics_get_interface_name(ics, &tmpstr) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
        free(tmpstr);
    }
    if (set_interface_name) {
        if (ics_set_interface_name(ics, set_interface_name) != 0)
            goto done;
    }

    /* comm_timeout */
    if (get_comm_timeout) {
        unsigned int timeout;

        if (ics_get_comm_timeout(ics, &timeout) != 0)
            goto done;
        fprintf(stderr, "%s: %u\n", prog, timeout);
    }
    if (set_comm_timeout != -1) {
        if (ics_set_comm_timeout(ics, set_comm_timeout) != 0)
            goto done;
    }

    /* static_ip_mode */
    if (get_static_ip_mode) {
        int flag;

        if (ics_get_static_ip_mode(ics, &flag) != 0)
            goto done;
        fprintf(stderr, "%s: %u\n", prog, flag);
    }
    if (set_static_ip_mode != -1) {
        if (ics_set_static_ip_mode(ics, set_static_ip_mode) != 0)
            goto done;
    }
    
    /* ip_number */
    if (get_ip_number) {
        char *tmpstr;

        if (ics_get_ip_number(ics, &tmpstr) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
        free(tmpstr);
    }
    if (set_ip_number) {
        if (ics_set_ip_number(ics, set_ip_number) != 0)
            goto done;
    }

    /* netmask */
    if (get_netmask) {
        char *tmpstr;

        if (ics_get_netmask(ics, &tmpstr) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
        free(tmpstr);
    }
    if (set_netmask) {
        if (ics_set_netmask(ics, set_netmask) != 0)
            goto done;
    }

    /* gateway */
    if (get_gateway) {
        char *tmpstr;

        if (ics_get_gateway(ics, &tmpstr) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
        free(tmpstr);
    }
    if (set_gateway) {
        if (ics_set_gateway(ics, set_gateway) != 0)
            goto done;
    }

    /* keepalive */
    if (get_keepalive) {
        unsigned int t;

        if (ics_get_keepalive(ics, &t) != 0)
            goto done;
        fprintf(stderr, "%s: %u\n", prog, t);
    }
    if (set_keepalive != -1) {
        if (ics_set_keepalive(ics, set_keepalive) != 0)
            goto done;
    }

    /* gpib_address */
    if (get_gpib_address) {
        unsigned int a;

        if (ics_get_gpib_address(ics, &a) != 0)
            goto done;
        fprintf(stderr, "%s: %u\n", prog, a);
    }
    if (set_gpib_address != -1) {
        if (ics_set_gpib_address(ics, set_gpib_address) != 0)
            goto done;
    }

    /* system_controller */
    if (get_system_controller) {
        int flag;

        if (ics_get_system_controller(ics, &flag) != 0)
            goto done;
        fprintf(stderr, "%s: %u\n", prog, flag);
    }
    if (set_system_controller != -1) {
        if (ics_set_system_controller(ics, set_system_controller) != 0)
            goto done;
    }

    /* ren_mode */
    if (get_ren_mode) {
        int flag;

        if (ics_get_ren_mode(ics, &flag) != 0)
            goto done;
        fprintf(stderr, "%s: %u\n", prog, flag);
    }
    if (set_ren_mode != -1) {
        if (ics_set_ren_mode(ics, set_ren_mode) != 0)
            goto done;
    }

    /* reload_factory */
    if (reload_factory) {
        if (ics_reload_factory(ics) != 0)
            goto done;
        fprintf(stderr, "%s: factory config reloaded\n", prog);
    }

    /* reload_config */
    if (reload_config) {
        if (ics_reload_config(ics) != 0)
            goto done;
        fprintf(stderr, "%s: default config reloaded\n", prog);
    }

    /* commit_config */
    if (commit_config) {
        if (ics_commit_config(ics) != 0)
            goto done;
        fprintf(stderr, "%s: current config written\n", prog);
    }

    /* get_errlog */
    if (get_errlog) {
        unsigned int *tmperr;
        int i, tmpcount = 0;

        if (ics_error_logger(ics, &tmperr, &tmpcount) != 0)
            goto done;            
        for (i = 0; i < tmpcount ; i++)
            fprintf(stderr, "%s: %s\n", prog, findstr(errtab, tmperr[i]));
        free(tmperr);
    }

    /* reboot */
    if (reboot) {
        if (ics_reboot(ics) != 0)
            goto done;
        fprintf(stderr, "%s: rebooting\n", prog);
    }

done:
    ics_fini(ics);
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

