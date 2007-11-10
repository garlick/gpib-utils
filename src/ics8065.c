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

#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "errstr.h"
#include "ics.h"

#define OPTIONS "n:resgiIj:cfCtT:mM:sS:zZ:kK:"
static struct option longopts[] = {
        {"name",                required_argument,  0, 'n'},
        {"get-interface-name",  no_argument,        0, 'I'},
        {"set-interface-name",  required_argument,  0, 'j'},
        {"get-comm-timeout",    no_argument,        0, 't'},
        {"set-comm-timeout",    required_argument,  0, 'T'},
        {"get-static-ip-mode",  no_argument,        0, 's'},
        {"set-static-ip-mode",  required_argument,  0, 'S'},
        {"get-ip-number",       no_argument,        0, 'z'},
        {"set-ip-number",       required_argument,  0, 'Z'},
        {"get-netmask",         no_argument,        0, 'k'},
        {"set-netmask",         required_argument,  0, 'K'},
        {"get-ren-mode",        no_argument,        0, 'm'},
        {"set-ren-mode",        required_argument,  0, 'M'},
        {"reload-config",       no_argument,        0, 'C'},
        {"reload-factory",      no_argument,        0, 'f'},
        {"commit-config",       no_argument,        0, 'c'},
        {"reboot",              no_argument,        0, 'r'},
        {"get-idn-string",      no_argument,        0, 'i'},
        {"errlog",              no_argument,        0, 'e'},
        {0, 0, 0, 0}
};

static errstr_t errtab[] = ICS_ERRLOG;
char *prog;

void
usage(void)
{
    fprintf(stderr, 
  "Usage: %s [--options]\n"
  "  -n,--name                 instrument hostname (default: gpibgw)\n"
  "  -I,--get-interface-name   get VXI-11 logical name (e.g. gpib0)\n"
  "  -j,--set-interface-name   set VXI-11 logical name (e.g. gpib0)\n"
  "  -t,--get-comm-timeout     get TCP timeout (in seconds)\n"
  "  -T,--set-comm-timeout     set TCP timeout (in seconds)\n"
  "  -s,--get-static-ip-mode   get static IP mode (0=disabled, 1=enabled)\n"
  "  -S,--set-static-ip-mode   set static IP mode (0=disabled, 1=enabled)\n"
  "  -z,--get-ip-number        get IP number (a.b.c.d)\n"
  "  -Z,--set-ip-number        set IP number (a.b.c.d)\n"
  "  -k,--get-netmask          get netmask (a.b.c.d)\n"
  "  -K,--set-netmask          set netmask (a.b.c.d)\n"
  "  -m,--get-ren-mode         get REN active at boot (0=false, 1=true)\n"
  "  -M,--set-ren-mode         set REN active at boot (0=false, 1=true)\n"
  "  -C,--reload-config        force reload of default config\n"
  "  -f,--reload-factory       reload factory config settings\n"
  "  -c,--commit-config        commit (write) current config\n"
  "  -r,--reboot               reboot\n"
  "  -i,--get-idn-string       get idn string\n"
  "  -e,--get-errlog           get error log (side effect: log is cleared)\n"
                       , prog);
        exit(1);

}

int
main(int argc, char *argv[])
{
    ics_t ics;
    int c;
    char *name = "gpibgw";
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
    int get_ren_mode = 0;
    int set_ren_mode = -1;
    int reload_config = 0;
    int reload_factory = 0;
    int commit_config = 0;
    int reboot = 0;
    int get_idn = 0;
    int get_errlog = 0;

    prog = basename(argv[0]);

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'n' :  /* --name */
                name = optarg;
                break;
            case 'I' :  /* --get-interface-name */
                get_interface_name = 1;
                break;
            case 'j' :  /* --set-interface-name */
                set_interface_name = optarg;
                break;
            case 't' :  /* --get-comm-timeout */
                get_comm_timeout = 1;
                break;
            case 'T' :  /* --set-comm-timeout */
                set_comm_timeout = strtoul(optarg, NULL, 0);
                break;
            case 's' :  /* --get-static-ip-mode */
                get_static_ip_mode = 1;
                break;
            case 'S' :  /* --set-static-ip-mode */
                set_static_ip_mode = strtoul(optarg, NULL, 0);
                break;
            case 'z' :  /* --get-ip-number */
                get_ip_number = 1;
                break;
            case 'Z' :  /* --set-ip-number */
                set_ip_number = optarg;
                break;
            case 'k' :  /* --get-netmask */
                get_ip_number = 1;
                break;
            case 'K' :  /* --set-netmask */
                set_netmask = optarg;
                break;
            case 'm' :  /* --get-ren-mode */
                get_ren_mode = 1;
                break;
            case 'M' :  /* --set-ren-mode */
                set_ren_mode = strtoul(optarg, NULL, 0);
                break;
            case 'C':   /* --reload-config */
                reload_config = 1;
                break;
            case 'f':   /* --reload-factory */
                reload_factory = 1;
                break;
            case 'c':   /* --commit-config */
                commit_config = 1;
                break;
            case 'r' :  /* --reboot */
                reboot = 1;
                break;
            case 'i' :  /* --idn */
                get_idn = 1;
                break;
            case 'e' :  /* --errlog */
                get_errlog = 1;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();

    if(!get_errlog && !get_idn && !reboot 
            && !commit_config && !reload_factory && !reload_config
            && !get_interface_name && !set_interface_name
            && !get_static_ip_mode && set_static_ip_mode == -1
            && !get_ip_number && !set_ip_number
            && !get_netmask && !set_netmask
            && !get_ren_mode && set_ren_mode == -1
            && !get_comm_timeout && set_comm_timeout == -1)
        usage();

    ics = ics_init(name);
    if (!ics)
        exit(1);

    if (get_idn) {
        char *tmpstr;

        if (ics_idn_string(ics, &tmpstr) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
    }
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
    if (reload_factory) {
        if (ics_reload_factory(ics) != 0)
            goto done;
        fprintf(stderr, "%s: factory config reloaded\n", prog);
    }
    if (reload_config) {
        if (ics_reload_config(ics) != 0)
            goto done;
        fprintf(stderr, "%s: default config reloaded\n", prog);
    }
    if (commit_config) {
        if (ics_commit_config(ics) != 0)
            goto done;
        fprintf(stderr, "%s: current config written\n", prog);
    }
    if (get_errlog) {
        unsigned int *tmperr;
        int i, tmpcount = 0;

        if (ics_error_logger(ics, &tmperr, &tmpcount) != 0)
            goto done;            
        for (i = 0; i < tmpcount ; i++)
            fprintf(stderr, "%s: %s\n", prog, finderr(errtab, tmperr[i]));
        free(tmperr);
    }

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

