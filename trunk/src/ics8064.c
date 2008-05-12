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
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#if HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "ics.h"
#include "util.h"
#include "gpib.h"
#include "hostlist.h"
#include "argv.h"

#define INSTRUMENT "ics8064"

#define OPTIONS "ea:cfCrjJ:tT:sS:zZ:kK:gG:nN:iv0:1:qQmIRlL"
static struct option longopts[] = {
        /* general */
        {"address",             required_argument,  0, 'a'},
        {"verbose",             no_argument,        0, 'v'},
        {"clear",               no_argument,        0, 'c'},
        {"local",               no_argument,        0, 'l'},

        /* these correspond to 8064-implemented ICS config RPCL functions */
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
        {"reload-factory",      no_argument,        0, 'f'},
        {"commit-config",       no_argument,        0, 'm'},
        {"get-errlog",          no_argument,        0, 'e'},
        {"reboot",              no_argument,        0, 'r'},

        /* vxi */
        {"get-idn-string",      no_argument,        0, 'i'},
        {"open",                required_argument,  0, '0'},
        {"close",               required_argument,  0, '1'},
        {"query",               no_argument,        0, 'q'},
        {"query-digital",       no_argument,        0, 'Q'},
        {"shell",               no_argument,        0, 'I'},
        {"radio",               no_argument,        0, 'R'},
        {"selftest",            no_argument,        0, 'L'},

        {0, 0, 0, 0}
};

static strtab_t errtab[] = ICS_ERRLOG;
char *prog;
static int radio = 0;

void
usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
  "Usage: %s [--options]\n"
  "  -n,--name name            instrument address [%s]\n"
  "  -c,--clear                set default values\n"
  "  -l,--local                return instrument to local operation on exit\n"
  "  -v,--verbose              show protocol on stderr\n"
  "  -j,--get-interface-name   get VXI-11 logical name (e.g. gpib0)\n"
  "  -J,--set-interface-name   set VXI-11 logical name (e.g. gpib0)\n"
  "  -t,--get-comm-timeout     get TCP timeout (in seconds)\n"
  "  -T,--set-comm-timeout     set TCP timeout (in seconds)\n"
  "  -s,--get-static-ip-mode   get static IP mode (0=disabled, 1=enabled)\n"
  "  -S,--set-static-ip-mode   set static IP mode (0=disabled, 1=enabled)\n"
  "  -z,--get-ip-number        get IP number (a.b.c.d)\n"
  "  -Z,--set-ip-number        set IP number (a.b.c.d)\n"
  "  -n,--get-netmask          get netmask (a.b.c.d)\n"
  "  -N,--set-netmask          set netmask (a.b.c.d)\n"
  "  -g,--get-gateway          get gateway (a.b.c.d)\n"
  "  -G,--set-gateway          set gateway (a.b.c.d)\n"
  "  -k,--get-keepalive        get keepalive time (in seconds)\n"
  "  -K,--set-keepalive        set keepalive time (in seconds)\n"
  "  -C,--reload-config        force reload of default config\n"
  "  -f,--reload-factory       reload factory config settings\n"
  "  -m,--commit-config        commit (write) current config\n"
  "  -r,--reboot               reboot\n"
  "  -e,--get-errlog           get error log (side effect: log is cleared)\n"
  "  -i,--get-idn-string       get idn string\n"
  "  -0,--open [targets]       open the specified relays\n"
  "  -1,--close [targets]      close the specified relays\n" 
  "  -q,--query                query the state of all relays\n"
  "  -Q,--query-digital        query the state of digital inputs\n"
  "  -I,--shell                start interactive shell\n"
  "  -L,--selftest             execute instrument selftest\n"
 "  -r,--radio                set radio button mode (max of one relay closed)\n"
                       , prog, addr ? addr : "no default");
        exit(1);
}

/* bit values for 8064 ESR register */
#define ICS8064_ESR_QUERY_ERR       0x4
#define ICS8064_ESR_FLASH_CORRUPT   0x8
#define ICS8064_ESR_EXEC_ERR        0x10
#define ICS8064_ESR_CMD_ERR         0x20

/* bit values for 8064 status register */
#define ICS8064_STB_ESR             0x20
#define ICS8064_STB_MAV             0x10


/* Interpet serial poll results (status byte)
 *   Return: 0=non-fatal/no error, >0=fatal, -1=retry.
 */
static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int esr, res = 0;
    char tmpstr[128];

    if ((status & ICS8064_STB_ESR)) {
        gpib_wrtf(gd, "*ESR\n");
        gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
        esr = strtoul(tmpstr, NULL, 10);
        if ((esr & ICS8064_ESR_QUERY_ERR))
            fprintf(stderr, "%s: %s: query error\n", prog, msg);
        if ((esr & ICS8064_ESR_FLASH_CORRUPT))
            fprintf(stderr, "%s: %s: flash corrupt\n", prog, msg);
        if ((esr & ICS8064_ESR_EXEC_ERR))
            fprintf(stderr, "%s: %s: execution error\n", prog, msg);
        if ((esr & ICS8064_ESR_CMD_ERR))
            fprintf(stderr, "%s: %s: command error\n", prog, msg);
        res = 1;
    }
    return res;
}

static void
_clear(gd_t gd)
{
    gpib_clr(gd, 1000);

    /* Reset to power-up state.
     * Allow 100ms after *RST per 8064 instruction manual.
     */
    gpib_wrtf(gd, "*RST\n"); 
    usleep(1000*100); 

    /* configure ESR bits that will reach STB reg */
    gpib_wrtf(gd, "*ESE %d\n", 
            (ICS8064_ESR_QUERY_ERR | ICS8064_ESR_FLASH_CORRUPT
           | ICS8064_ESR_EXEC_ERR  | ICS8064_ESR_CMD_ERR));
}

void
_selftest(gd_t gd)
{
    char tmpstr[128];
    int result;

    gpib_wrtstr(gd, "*TST?\n");
    gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
    result = strtoul(tmpstr, NULL, 10);
    if (result == 0)
        fprintf(stderr, "%s: self-test OK\n", prog);
    else /* will not happen - on error, 8064 will enter LED blink loop */
        fprintf(stderr, "%s: self-test returned %d\n", prog, result);
}

static void
_get_idn(gd_t gd)
{
    char tmpstr[64];

    gpib_wrtstr(gd, "*IDN?\n");
    gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
    fprintf(stderr, "%s: %s\n", prog, tmpstr);
}

static void
_query_digital(gd_t gd)
{
    char tmpstr[64];

    gpib_wrtstr(gd, "STAT:QUES:COND?\n");
    gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
    fprintf(stderr, "%s: %s\n", prog, tmpstr);
}

static void
_query_relays(gd_t gd)
{
    char tmpstr[128];

    gpib_wrtstr(gd, "ROUTE:CLOSE:STATE?\n");
    gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
    fprintf(stderr, "%s: %s\n", prog,tmpstr);
}

static void
_open_one_relay(gd_t gd, char *relay)
{
    gpib_wrtf(gd, "ROUTE:OPEN %s\n", relay);
}

static void
_open_relays(gd_t gd, char *targets)
{
    hostlist_t h = hostlist_create(targets);
    hostlist_iterator_t it = hostlist_iterator_create(h);
    char *relay;

    while ((relay = hostlist_next(it)))
        _open_one_relay(gd, relay);
    hostlist_iterator_destroy(it);
    hostlist_destroy(h);
}

static void
_close_one_relay(gd_t gd, char *relay)
{
    if (radio)
        gpib_wrtf(gd, "ROUTE:OPEN:ALL\n");
    gpib_wrtf(gd, "ROUTE:CLOSE %s\n", relay);
}

static void
_close_relays(gd_t gd, char *targets)
{
    hostlist_t h = hostlist_create(targets);
    hostlist_iterator_t it = hostlist_iterator_create(h);
    char *relay;

    while ((relay = hostlist_next(it)))
        _close_one_relay(gd, relay);
    hostlist_iterator_destroy(it);
    hostlist_destroy(h);
}

static int
_validate_targets(char *targets)
{
    hostlist_t h = hostlist_create(targets);
    hostlist_iterator_t it = hostlist_iterator_create(h);
    char *relay, *endptr;
    int n;
    int res = 1;

    while ((relay = hostlist_next(it))) {
        n = strtoul(relay, &endptr, 10);
        if (*endptr || n < 1 || n > 16) {
            fprintf(stderr, "%s: relays are number 1 - 16\n", prog);
            res = 0;
            break;
        }
    }
    hostlist_iterator_destroy(it);
    hostlist_destroy(h);
    return res;
}

#if HAVE_LIBREADLINE
static int
_docmd(gd_t gd, char **av)
{
    int rc = 0;

    if (av[0]) {
        if (!strcmp(av[0], "quit")) {
            rc = 1;

        } else if (!strcmp(av[0], "close")) {
            if (!av[1] || av[2])
                printf("Usage: close relay-list\n");
            else if (_validate_targets(av[1]))
                _close_relays(gd, av[1]);

        } else if (!strcmp(av[0], "open")) {
            if (!av[1] || av[2])
                printf("Usage: open relay-list\n");
            else if (_validate_targets(av[1]))
                _open_relays(gd, av[1]);
        } else if (!strcmp(av[0], "status")) {
            if (av[1])
                printf("Usage: status\n");
            else
                _query_relays(gd);
        } else if (!strcmp(av[0], "help")) {
            printf("Commands:\n");
            printf("open relay-list       open the specified relay(s)\n");
            printf("close relay-list      close the specified relay(s)\n");
            printf("status                show relay status\n");
            printf("quit                  exit interactive mode\n");
            printf("Relay-list is comma-separated and may include ranges,\n");
            printf(" e.g.: \"1,2\", \"1-16\", or \"1-10,16\".\n");
        } else {
            printf("Unknown command\n");
        }
    }
    return rc;
}

static void
_shell(gd_t gd)
{
    char *line;
    char **av;
    int rc = 0;

    /* disable readline file name completion */
    /*rl_bind_key ('\t', rl_insert); */

    while (rc == 0 && (line = readline("ics8064> "))) {
        if (strlen(line) > 0) {
            add_history(line);
            av = argv_create(line, "");
            rc = _docmd(gd, av);
            argv_destroy(av);
        }
        free(line);
    }
}
#else
static void
_shell(gd_t gd)
{
    fprintf(stderr, "%s: not configured with readline support\n", prog);
}
#endif /* HAVE_LIBREADLINE */

int
main(int argc, char *argv[])
{
    ics_t ics = NULL;
    gd_t gd = NULL;
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
    int reload_config = 0;
    int reload_factory = 0;
    int commit_config = 0;
    int reboot = 0;
    int get_idn = 0;
    int get_errlog  = 0;
    int ics_todo = 0;
    int vxi_todo = 0;
    int verbose = 0;
    char *open = NULL;
    char *close = NULL;
    int query = 0;
    int query_dig = 0;
    int clear = 0;
    int shell = 0;
    int local = 0;
    int selftest = 0;

    prog = basename(argv[0]);

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'a' :  /* --address */
                addr = optarg;
                break;
            case 'v' :  /* --verbose */
                verbose++;
                break;
            case 'l' :  /* --local */
                vxi_todo++;
                local++;
                break;
            case 'c' :  /* --clear */
                clear++;
                vxi_todo++;
                break;
            case 'j' :  /* --get-interface-name */
                get_interface_name = 1;
                ics_todo++;
                break;
            case 'J' :  /* --set-interface-name */
                set_interface_name = optarg;
                ics_todo++;
                break;
            case 't' :  /* --get-comm-timeout */
                get_comm_timeout = 1;
                ics_todo++;
                break;
            case 'T' :  /* --set-comm-timeout */
                set_comm_timeout = strtoul(optarg, NULL, 0);
                ics_todo++;
                break;
            case 's' :  /* --get-static-ip-mode */
                get_static_ip_mode = 1;
                ics_todo++;
                break;
            case 'S' :  /* --set-static-ip-mode */
                set_static_ip_mode = strtoul(optarg, NULL, 0);
                ics_todo++;
                break;
            case 'z' :  /* --get-ip-number */
                get_ip_number = 1;
                ics_todo++;
                break;
            case 'Z' :  /* --set-ip-number */
                set_ip_number = optarg;
                ics_todo++;
                break;
            case 'n' :  /* --get-netmask */
                get_netmask = 1;
                ics_todo++;
                break;
            case 'N' :  /* --set-netmask */
                set_netmask = optarg;
                ics_todo++;
                break;
            case 'g' :  /* --get-gateway */
                get_gateway = 1;
                ics_todo++;
                break;
            case 'G' :  /* --set-gateway */
                set_gateway = optarg;
                ics_todo++;
                break;
            case 'k' :  /* --get-keepalive */
                get_keepalive = 1;
                ics_todo++;
                break;
            case 'K' :  /* --set-keepalive */
                set_keepalive = strtoul(optarg, NULL, 0);
                ics_todo++;
                break;
            case 'C':   /* --reload-config */
                reload_config = 1;
                ics_todo++;
                break;
            case 'f':   /* --reload-factory */
                reload_factory = 1;
                ics_todo++;
                break;
            case 'm':   /* --commit-config */
                commit_config = 1;
                ics_todo++;
                break;
            case 'e' :  /* --get-errlog */
                get_errlog = 1;
                ics_todo++;
                break;
            case 'r' :  /* --reboot */
                reboot = 1;
                ics_todo++;
                break;
            case 'i' :  /* --get-idn-string */
                get_idn = 1;
                vxi_todo++;
                break;
            case '0' :  /* --open */
                open = optarg;
                vxi_todo++;
                break;
            case '1' :  /* --close */
                close = optarg;
                vxi_todo++;
                break;
            case 'q' :  /* --query */
                query = 1;
                vxi_todo++;
                break;
            case 'Q' :  /* --query-dig */
                query_dig = 1;
                vxi_todo++;
                break;
            case 'I' :  /* --shell */
                shell++;
                vxi_todo++;
                break;
            case 'R' :  /* --radio */
                radio++;
                break;
            case 'L' :  /* --selftest */
                selftest++;
                vxi_todo++;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc || (!ics_todo && !vxi_todo))
        usage();

    if (!addr)
        addr = gpib_default_addr(INSTRUMENT);
    if (!addr) {
        fprintf(stderr, "%s: no default address for %s, use --address\n",
                prog, INSTRUMENT);
        exit(1);
    }

    /** ICS config funtions.
     **/

    if (ics_todo) {
        ics = ics_init(addr);
        if (!ics)
            exit(1);
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

    /** Now vxi operations.
     **/

    if (vxi_todo) {
        gd = gpib_init(addr , _interpret_status, 100000);
        if (!gd) {
            fprintf(stderr, "%s: device initialization failed for address %s\n",
                    prog, addr);
            exit(1);
        }
        if (verbose)
            gpib_set_verbose(gd, 1);
    }
    if (clear)
        _clear(gd);
    if (selftest)
        _selftest(gd);
    if (get_idn)
        _get_idn(gd);
    if (open) {
        if (!_validate_targets(open))
            goto done;
        _open_relays(gd, open);
    }
    if (close) {
        if (!_validate_targets(close))
            goto done;
        _close_relays(gd, close);
    }
    if (query)
        _query_relays(gd);
    if (query_dig)
        _query_digital(gd);
    if (shell)
        _shell(gd);
    if (local)
        gpib_loc(gd);

done:
    if (ics)
        ics_fini(ics);
    if (gd)
        gpib_fini(gd);
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
