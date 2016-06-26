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
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libutil/util.h"
#include "liblsd/hostlist.h"
#include "libutil/argv.h"

#include "libinst/inst.h"
#include "libinst/configfile.h"

#define INSTRUMENT "ics8064"

/* bit values for 8064 ESR register */
#define ICS8064_ESR_QUERY_ERR       0x4
#define ICS8064_ESR_FLASH_CORRUPT   0x8
#define ICS8064_ESR_EXEC_ERR        0x10
#define ICS8064_ESR_CMD_ERR         0x20

/* bit values for 8064 status register */
#define ICS8064_STB_ESR             0x20
#define ICS8064_STB_MAV             0x10

const char *options = "n:a:vcli0:1:qQIRL";
static struct option longopts[] = {
        {"name",                required_argument,  0, 'n'},
        {"address",             required_argument,  0, 'a'},
        {"verbose",             no_argument,        0, 'v'},
        {"clear",               no_argument,        0, 'c'},
        {"local",               no_argument,        0, 'l'},
        {"get-idn-string",      no_argument,        0, 'i'},
        {"open",                required_argument,  0, '0'},
        {"close",               required_argument,  0, '1'},
        {"query",               no_argument,        0, 'q'},
        {"query-digital",       no_argument,        0, 'Q'},
        {"shell",               no_argument,        0, 'I'},
        {"radio",               no_argument,        0, 'R'},
        {"selftest",            no_argument,        0, 'L'},
        { 0, 0, 0 },
};

void usage (void)
{
    fprintf (stderr, "%s",
        "    -n,--name           specify instrument name (default 3488a)\n"
        "    -a,--address        specify instrument address\n"
        "    -v,--verbose        set verbose mode\n"
        "    -c,--clear          initialize instrument to default values\n"
        "    -l,--local          return instrument to local operation on exit\n"
        "    -i,--get-idn-string get idn string\n"
        "    -0,--open           open the specified relays\n"
        "    -1,--close          close the specified relays\n"
        "    -q,--query          query the state of all relays\n"
        "    -Q,--query-digital  query the state of digital inputs\n"
        "    -I,--shell          start interactive shell\n"
        "    -L,--selftest       execute instrument selftest\n"
        "    -R,--radio          set radio button mode (max one relay closed)\n");
    exit (1);
};

static int _interpret_status(struct instrument *gd, unsigned char status, char *msg);
static void _clear(struct instrument *gd);
static void _query_relays(struct instrument *gd);
static void _open_relays(struct instrument *gd, char *targets);
static void _close_relays(struct instrument *gd, char *targets);
static int _validate_targets(char *targets);
static void _shell(struct instrument *gd);
static void _get_idn(struct instrument *gd);
static void _query_digital(struct instrument *gd);
static void _selftest(struct instrument *gd);

char *prog;
static int radio = 0;

int
main(int argc, char *argv[])
{
    struct instrument *gd;
    int c;
    int exit_val = 0;
    char *name = INSTRUMENT;
    char *address = NULL;

    prog = basename (argv[0]);

    while ((c = getopt_long (argc, argv, options, longopts, NULL)) != EOF) {
        switch(c) {
            case 'R' :  /* --radio */
                radio++;
                break;
            case 'n':
                name = optarg;
                break;
            case 'a':
                address = optarg;
                break;
            default:
                break;
        }
    }

    if (!address) {
        struct cf_file *cf = cf_create_default ();
        const struct cf_instrument *cfi = NULL;

        if (cf)
            cfi = cf_lookup (cf, name);
        if (!cfi) {
            fprintf (stderr, "Please supply --address or cf entry for [%s]\n",
                     name);
            exit (1);
        }
        gd = inst_init (cfi->addr, _interpret_status, 100000);
        if (!gd) {
            fprintf (stderr, "Failed to initialize instrument\n");
            exit (1);
        }
        cf_destroy (cf);
    } else {
        gd = inst_init (address, _interpret_status, 100000);
        if (!gd) {
            fprintf (stderr, "Failed to initialize instrument\n");
            exit (1);
        }
    }

    optind = 0;
    while ((c = getopt_long (argc, argv, options, longopts, NULL)) != EOF) {
        switch (c) {
            case 'R':
            case 'n':
            case 'a':   /* handled above */
                break;
            case 'l' :  /* --local */
                inst_loc(gd);
                break;
            case 'c' :  /* --clear */
                _clear(gd);
                break;
            case 'i' :  /* --get-idn-string */
                _get_idn(gd);
                break;
            case '0' :  /* --open */
                if (_validate_targets(optarg))
                    _open_relays(gd, optarg);
                else
                    exit_val = 1;
                break;
            case '1' :  /* --close */
                if (_validate_targets(optarg))
                    _close_relays(gd, optarg);
                else
                    exit_val = 1;
                break;
            case 'q' :  /* --query */
                _query_relays(gd);
                break;
            case 'Q' :  /* --query-dig */
                _query_digital(gd);
                break;
            case 'I' :  /* --shell */
                _shell(gd);
                break;
            case 'L' :  /* --selftest */
                _selftest(gd);
                break;
            default:
                usage ();
        }
    }

    inst_fini (gd);
    exit (exit_val);
}


/* Interpet serial poll results (status byte)
 *   Return: 0=non-fatal/no error, >0=fatal, -1=retry.
 */
static int
_interpret_status(struct instrument *gd, unsigned char status, char *msg)
{
    int esr, res = 0;
    char tmpstr[128];

    if ((status & ICS8064_STB_ESR)) {
        inst_wrtf(gd, "*ESR\n");
        inst_rdstr(gd, tmpstr, sizeof(tmpstr));
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
_clear(struct instrument *gd)
{
    inst_clr(gd, 1000);

    /* Reset to power-up state.
     * Allow 100ms after *RST per 8064 instruction manual.
     */
    inst_wrtf(gd, "*RST\n");
    usleep(1000*100);

    /* configure ESR bits that will reach STB reg */
    inst_wrtf(gd, "*ESE %d\n",
            (ICS8064_ESR_QUERY_ERR | ICS8064_ESR_FLASH_CORRUPT
           | ICS8064_ESR_EXEC_ERR  | ICS8064_ESR_CMD_ERR));
}

static void
_selftest(struct instrument *gd)
{
    char tmpstr[128];
    int result;

    inst_wrtstr(gd, "*TST?\n");
    inst_rdstr(gd, tmpstr, sizeof(tmpstr));
    result = strtoul(tmpstr, NULL, 10);
    if (result == 0)
        fprintf(stderr, "%s: self-test OK\n", prog);
    else /* will not happen - on error, 8064 will enter LED blink loop */
        fprintf(stderr, "%s: self-test returned %d\n", prog, result);
}

static void
_get_idn(struct instrument *gd)
{
    char tmpstr[64];

    inst_wrtstr(gd, "*IDN?\n");
    inst_rdstr(gd, tmpstr, sizeof(tmpstr));
    fprintf(stderr, "%s: %s\n", prog, tmpstr);
}

static void
_query_digital(struct instrument *gd)
{
    char tmpstr[64];

    inst_wrtstr(gd, "STAT:QUES:COND?\n");
    inst_rdstr(gd, tmpstr, sizeof(tmpstr));
    fprintf(stderr, "%s: %s\n", prog, tmpstr);
}

static void
_query_relays(struct instrument *gd)
{
    char tmpstr[128];

    inst_wrtstr(gd, "ROUTE:CLOSE:STATE?\n");
    inst_rdstr(gd, tmpstr, sizeof(tmpstr));
    fprintf(stderr, "%s: %s\n", prog,tmpstr);
}

static void
_open_one_relay(struct instrument *gd, char *relay)
{
    inst_wrtf(gd, "ROUTE:OPEN %s\n", relay);
}

static void
_open_relays(struct instrument *gd, char *targets)
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
_close_one_relay(struct instrument *gd, char *relay)
{
    if (radio)
        inst_wrtf(gd, "ROUTE:OPEN:ALL\n");
    inst_wrtf(gd, "ROUTE:CLOSE %s\n", relay);
}

static void
_close_relays(struct instrument *gd, char *targets)
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

static int
_docmd(struct instrument *gd, char **av)
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
_shell(struct instrument *gd)
{
    char buf[128];
    char **av;
    int rc = 0;

    while (rc == 0 && xreadline("ics8064> ", buf, sizeof(buf))) {
        if (strlen(buf) > 0) {
            av = argv_create(buf, "");
            rc = _docmd(gd, av);
            argv_destroy(av);
        }
    }
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
