/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2008 Jim Garlick <garlick.jim@gmail.com>

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
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "libics/ics.h"
#include "libutil/util.h"

static void _list(ics_t ics, int argc, char **argv);
static void _set(ics_t ics, int argc, char **argv);
static void _get(ics_t ics, int argc, char **argv);
static void _reload(ics_t ics, int argc, char **argv);
static void _factory(ics_t ics, int argc, char **argv);
static void _commit(ics_t ics, int argc, char **argv);
static void _errlog(ics_t ics, int argc, char **argv);
static void _reboot(ics_t ics, int argc, char **argv);
static void _usage(void);

static strtab_t errtab[] = ICS_ERRLOG;
char *prog;

int
main(int argc, char *argv[])
{
    ics_t ics = NULL;
    char *name, *cmd;
    int rc, optind = 0;

    prog = basename(argv[optind++]);

    if (argc - optind < 2)
        _usage ();
    name = argv[optind++];
    cmd = argv[optind++];

    if ((rc = ics_init(name, &ics)) != 0) {
        fprintf (stderr, "%s: %s: %s\n", prog, name, ics_strerror (ics, rc));
        exit (1);
    }

    if (!strcmp (cmd, "list")) {
        _list(ics, argc - optind, argv + optind);
    } else if (!strcmp (cmd, "get")) {
        _get(ics, argc - optind, argv + optind);
    } else if (!strcmp (cmd, "set")) {
        _set(ics, argc - optind, argv + optind);
    } else if (!strcmp (cmd, "reload")) {
        _reload(ics, argc - optind, argv + optind);
    } else if (!strcmp (cmd, "factory")) {
        _factory(ics, argc - optind, argv + optind);
    } else if (!strcmp (cmd, "commit")) {
        _commit(ics, argc - optind, argv + optind);
    } else if (!strcmp (cmd, "errlog")) {
        _errlog(ics, argc - optind, argv + optind);
    } else if (!strcmp (cmd, "reboot")) {
        _reboot(ics, argc - optind, argv + optind);
    } else
        _usage ();

    ics_fini(ics);

    exit(0);
}

static void
_usage(void)
{
    fprintf(stderr, 
  "Usage: icsconfig DEVICE CMD [OPTIONS]\n"
  "  where CMD is:\n"
  "  list             - display config\n"
  "  get NAME         - display value of config attribute NAME\n"
  "  set NAME=VALUE   - set config attribute NAME to VALUE\n"
  "  reload           - reload config from flash\n"
  "  factory          - reload flash with factory defaults\n"
  "  commit           - save config to flash for next reboot\n"
  "  errlog           - display device error log\n"
  "  reboot           - reboot device\n");
    exit (1);
}

static const char *attrs[] = {
    "ident",
    "enable-dhcp",
    "ip-address",
    "ip-gateway",
    "ip-netmask",
    "hostname",
    "timeout",
    "keepalive",
    "vxi11-interface",
    "gpib-address",
    "gpib-controller",
    "gpib-ren-enable",
    NULL,
};

static void
_print_attribute (ics_t ics, const char *name)
{
    int rc = 0;

    printf("%-20s", name);

    if (!strcmp (name, "ident")) {
        char *s;
        if ((rc = ics_idn_string(ics, &s)) == 0) {
            printf("%s\n", s);
            free(s);
        }
    } else if (!strcmp (name, "timeout")) {
        unsigned int t;
        rc = ics_get_comm_timeout(ics, &t);
        if (rc == 0)
            printf("%u\n", t);
    } else if (!strcmp (name, "enable-dhcp")) {
        int mode;
        rc = ics_get_static_ip_mode(ics, &mode);
        if (rc == 0);
            printf("%s\n", mode == 0 ? "yes" : "no");
    } else if (!strcmp (name, "ip-address")) {
        char *s;
        rc = ics_get_ip_number(ics, &s);
        if (rc == 0) {
            printf("%s\n", s);
            free(s);
        }
    } else if (!strcmp (name, "ip-netmask")) {
        char *s;
        rc = ics_get_netmask(ics, &s);
        if (rc == 0) {
            printf("%s\n", s);
            free(s);
        }
    } else if (!strcmp (name, "ip-gateway")) {
        char *s;
        rc = ics_get_gateway(ics, &s);
        if (rc == 0) {
            printf("%s\n", s);
            free(s);
        }
    } else if (!strcmp (name, "hostname")) {
        char *s;
        if ((rc = ics_get_hostname(ics, &s)) == 0) {
            printf("%s\n", s);
            free(s);
        }
    } else if (!strcmp (name, "keepalive")) {
        unsigned int t;
        rc = ics_get_keepalive(ics, &t);
        if (rc == 0)
            printf("%u\n", t);
    } else if (!strcmp (name, "vxi11-interface")) {
        char *s;
		rc = ics_get_interface_name(ics, &s);
		if (rc == 0) {
		    printf("%s\n", s);
		    free(s);
        }
    } else if (!strcmp (name, "gpib-address")) {
        unsigned int addr;
        rc = ics_get_gpib_address(ics, &addr);
        if (rc == 0)
            printf("%u\n", addr);
    } else if (!strcmp (name, "gpib-controller")) {
        int mode;
        rc = ics_get_system_controller(ics, &mode);
        if (rc == 0)
            printf("%s\n", mode == 0 ? "no" : "yes");
    } else if (!strcmp (name, "gpib-ren-enable")) {
        int mode;
        rc = ics_get_ren_mode(ics, &mode);
        if (rc == 0)
            printf("%s\n", mode == 0 ? "no" : "yes");
    } else
        printf("?\n");
    if (rc == ICS_ERROR_UNSUPPORTED)
        printf("n/a\n");
    else if (rc != 0)
        printf("%s\n", ics_strerror (ics, rc));
}

static void
_list(ics_t ics, int argc, char **argv)
{
    int n = 0;
    while (attrs[n])
        _print_attribute (ics, attrs[n++]);
}

static void
_get(ics_t ics, int argc, char **argv)
{
    int n = 0;
    if (argc < 1)
        _usage ();
    while (n < argc)
        _print_attribute (ics, argv[n++]);
}

static void
_set(ics_t ics, int argc, char **argv)
{
    char *name, *value;
    char *cpy = NULL;
    int rc = 0;

    if (argc == 2) {
        name = argv[0];
        value = argv[1];
    } else if (argc == 1) {
        name = cpy = strdup (argv[0]);
        if (!name) {
            fprintf (stderr, "out of memory\n");
            exit (1);
        }
        if ((value = strchr (name, '=')))
            *value++ = '\0';
        else
            _usage();
    } else
        _usage ();

    if (!strcmp (name, "timeout")) {
        unsigned int n = strtoul (value, NULL, 10);
        rc = ics_set_comm_timeout(ics, n);
    } else if (!strcmp (name, "enable-dhcp")) {
        int n = !strcmp (value, "yes") ? 0 : 1;
        rc = ics_set_static_ip_mode(ics, n);
    } else if (!strcmp (name, "ip-address")) {
        rc = ics_set_ip_number(ics, value);
    } else if (!strcmp (name, "ip-netmask")) {
        rc = ics_set_netmask(ics, value);
    } else if (!strcmp (name, "ip-gateway")) {
        rc = ics_set_gateway(ics, value);
    } else if (!strcmp (name, "hostname")) {
        rc = ics_set_hostname(ics, value);
    } else if (!strcmp (name, "keepalive")) {
        unsigned int n = strtoul (value, NULL, 10);
        rc = ics_set_keepalive(ics, n);
    } else if (!strcmp (name, "vxi11-interface")) {
        rc = ics_set_interface_name(ics, value);
    } else if (!strcmp (name, "gpib-address")) {
        unsigned int addr = strtoul (value, NULL, 0);
        rc = ics_set_gpib_address(ics, addr);
    } else if (!strcmp (name, "gpib-controller")) {
        int n = !strcmp (value, "yes") ? 1 : 0;
        rc = ics_set_system_controller(ics, n);
    } else if (!strcmp (name, "gpib-ren-enable")) {
        int n = !strcmp (value, "yes") ? 1 : 0;
        rc = ics_set_ren_mode(ics, n);
    } else if (!strcmp (name, "ident")) {
        fprintf(stderr, "ident is read-only\n");
    } else
        fprintf(stderr, "unknown attribute\n");

    if (rc != 0)
        fprintf(stderr, "set: %s: %s\n", name, ics_strerror (ics, rc));

    if (cpy)
        free (cpy);
}

static void
_reload(ics_t ics, int argc, char **argv)
{
    int rc;
    if (argc > 0)
        _usage();
    if ((rc = ics_reload_config(ics)) == 0)
        printf("config reloaded from flash\n");
    else
        fprintf(stderr, "reload: %s\n", ics_strerror (ics, rc));
}

static void
_factory(ics_t ics, int argc, char **argv)
{
    int rc;
    if (argc > 0)
        _usage();
    if ((rc = ics_reload_config(ics)) == 0)
        printf("flash reloaded with factory defaults\n");
    else
        fprintf(stderr, "factory: %s\n", ics_strerror (ics, rc));
}

static void
_commit(ics_t ics, int argc, char **argv)
{
    int rc;
    if (argc > 0)
        _usage();
    if ((rc = ics_commit_config(ics)) == 0)
        printf("current config written\n");
    else
        fprintf(stderr, "commit: %s\n", ics_strerror (ics, rc));
}

static void
_errlog(ics_t ics, int argc, char **argv)
{
    unsigned int *tmperr;
    int i, tmpcount = 0;
    int rc;

    if (argc > 0)
        _usage();
    if ((rc = ics_error_logger(ics, &tmperr, &tmpcount)) == 0) {
        for (i = 0; i < tmpcount ; i++)
            printf("[%d] %s\n", i, findstr(errtab, tmperr[i]));
        free(tmperr);
    } else
        fprintf(stderr, "errlog: %s\n", ics_strerror (ics, rc));
}

static void
_reboot(ics_t ics, int argc, char **argv)
{
    int rc;
    if (argc > 0)
        _usage();
    if ((rc = ics_reboot(ics)) == 0)
        printf("rebooting\n");
    else
        fprintf(stderr, "reboot: %s\n", ics_strerror (ics, rc));
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
