/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2008-2009 Jim Garlick <garlick.jim@gmail.com>

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
#if HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ics.h"
#include "util.h"

#define OPTIONS "a:jJ:tT:sS:zZ:nN:gG:kK:CceriqQ:wW:mM:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
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
        {"get-netmask",         no_argument,        0, 'n'},
        {"set-netmask",         required_argument,  0, 'N'},
        {"get-gateway",         no_argument,        0, 'g'},
        {"set-gateway",         required_argument,  0, 'G'},
        {"get-keepalive",       no_argument,        0, 'k'},
        {"set-keepalive",       required_argument,  0, 'K'},
        {"reload-config",       no_argument,        0, 'C'},
        {"commit-config",       no_argument,        0, 'c'},
        {"get-errlog",          no_argument,        0, 'e'},
        {"reboot",              no_argument,        0, 'r'},
        {"get-idn-string",      no_argument,        0, 'i'},
        {"get-gpib-address",    no_argument,        0, 'q'},
        {"set-gpib-address",    required_argument,  0, 'Q'},
        {"get-system-controller",no_argument,       0, 'w'},
        {"set-system-controller",required_argument, 0, 'W'},
        {"get-ren-mode",        no_argument,        0, 'm'},
        {"set-ren-mode",        required_argument,  0, 'M'},

        {0, 0, 0, 0}
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static void _print_errlog(ics_t ics);

static strtab_t errtab[] = ICS_ERRLOG;
char *prog;

void
usage(void)
{
    fprintf(stderr, 
  "Usage: %s [--options]\n"
  "  -a,--name                    instrument address\n"
  "  -jJ,--get/set-interface-name VXI-11 logical name (e.g. ``inst'')\n"
  "  -tT,--get/set-comm-timeout   TCP timeout (in seconds)\n"
  "  -sS,--get/set-static-ip-mode static IP mode (0=disabled, 1=enabled)\n"
  "  -zZ,--get/set-ip-number      static IP number (a.b.c.d)\n"
  "  -nN,--get/set-netmask        static netmask (a.b.c.d)\n"
  "  -gG,--get/set-gateway        static gateway (a.b.c.d)\n"
  "  -kK,--get/set-keepalive      keepalive time (in seconds)\n"
  "  -C,--reload-config           force reload of default config\n"
  "  -c,--commit-config           commit (write) current config\n"
  "  -r,--reboot                  reboot\n"
  "  -e,--get-errlog              get error log (side effect: log is cleared)\n"
  "The following options apply to the ICS 8065 Ethernet-GPIB controller only:\n"
  "  -qQ,--get/set-gpib-address   gpib address\n"
  "  -wW,--get/set-system-controller\n"
  "                               sys controller mode (0=disabled, 1=enabled)\n"
  "  -mM,--get/set-ren-mode       REN active at boot (0=false, 1=true)\n"
  "  -i,--get-idn-string          get idn string\n", prog);
}

int
main(int argc, char *argv[])
{
    ics_t ics = NULL;
    char *addr = NULL;
    char *tmpstr;
    unsigned int timeout, a;
    int flag, c;

    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'a' :  /* --address */
                addr = optarg;
                break;
        }
    }	
    if (!addr) {
        usage();
        exit(1);
    }
    if (!(ics = ics_init(addr)))
        exit(1);

    optind = 0;
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'a': /* --address already handled above */
                break;
            case 'j' : /* --get-interface-name */
		if (ics_get_interface_name(ics, &tmpstr) == 0) {
		    printf("interface-name: %s\n", tmpstr);
		    free(tmpstr);
                }
                break;
            case 'J' :  /* --set-interface-name */
                ics_set_interface_name(ics, optarg);
                break;
            case 't' :  /* --get-comm-timeout */
                if (ics_get_comm_timeout(ics, &timeout) == 0)
                    printf("comm-timeout: %u\n", timeout);
                break;
            case 'T' :  /* --set-comm-timeout */
                ics_set_comm_timeout(ics, strtoul(optarg, NULL, 0));
                break;
            case 's' :  /* --get-static-ip-mode */
                if (ics_get_static_ip_mode(ics, &flag) == 0)
                    printf("static-ip-mode: %u\n", flag);
                break;
            case 'S' :  /* --set-static-ip-mode */
                ics_set_static_ip_mode(ics, strtoul(optarg, NULL, 0));
                break;
            case 'z' :  /* --get-ip-number */
                if (ics_get_ip_number(ics, &tmpstr) == 0) {
                    printf("ip-number: %s\n", tmpstr);
                    free(tmpstr);
                }
                break;
            case 'Z' :  /* --set-ip-number */
                ics_set_ip_number(ics, optarg);
                break;
            case 'n' :  /* --get-netmask */
                if (ics_get_netmask(ics, &tmpstr) == 0) {
                    printf("netmask: %s\n", tmpstr);
                    free(tmpstr);
                }
                break;
            case 'N' :  /* --set-netmask */
                ics_set_netmask(ics, optarg);
                break;
            case 'g' :  /* --get-gateway */
                if (ics_get_gateway(ics, &tmpstr) == 0) {
                    printf("gateway: %s\n", tmpstr);
                    free(tmpstr);
                }
                break;
            case 'G' :  /* --set-gateway */
                ics_set_gateway(ics, optarg);
                break;
            case 'k' :  /* --get-keepalive */
                if (ics_get_keepalive(ics, &timeout) == 0)
                    printf("keepalive: %u\n", timeout);
                break;
            case 'K' :  /* --set-keepalive */
                ics_set_keepalive(ics, strtoul(optarg, NULL, 0));
                break;
            case 'C':   /* --reload-config */
                if (ics_reload_config(ics) == 0)
                    printf("default config reloaded\n");
                break;
            case 'c':   /* --commit-config */
                if (ics_commit_config(ics) == 0)
                    printf("current config written\n");
                break;
            case 'e' :  /* --get-errlog */
                _print_errlog(ics);
                break;
            case 'r' :  /* --reboot */
                if (ics_reboot(ics) == 0)
                    printf("rebooting\n");
                break;
            case 'i' :  /* --get-idn-string */
                if (ics_idn_string(ics, &tmpstr) == 0) {
                    printf("idn-string: %s\n", tmpstr);
                    free(tmpstr);
                }
                break;
            case 'q' :  /* --get-gpib-address */
                if (ics_get_gpib_address(ics, &a) == 0)
                    printf("gpib-address: %u\n", a);
                break;
            case 'Q' :  /* --set-gpib-address */
                ics_set_gpib_address(ics, strtoul(optarg, NULL, 0));
                break;
            case 'w' :  /* --get-system-controller */
                if (ics_get_system_controller(ics, &flag) == 0)
                    printf("system-controller: %u\n", flag);
                break;
            case 'W' :  /* --set-system-controller */
                ics_set_system_controller(ics, strtoul(optarg, NULL, 0));
                break;
            case 'm':   /* --get-ren-mode */
                if (ics_get_ren_mode(ics, &flag) == 0)
                    printf("ren-mode: %u\n", flag);
                break;
            case 'M':   /* --set-ren-mode */
                ics_set_ren_mode(ics, strtoul(optarg, NULL, 0));
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();
    ics_fini(ics);
    exit(0);
}

static void
_print_errlog(ics_t ics)
{
    unsigned int *tmperr;
    int i, tmpcount = 0;

    if (ics_error_logger(ics, &tmperr, &tmpcount) == 0) {
        for (i = 0; i < tmpcount ; i++)
            printf("[%d]: %s\n", i, findstr(errtab, tmperr[i]));
        free(tmperr);
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
