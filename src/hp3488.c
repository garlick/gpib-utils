/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005-2009 Jim Garlick <garlick.jim@gmail.com>

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

/* References:
 * "HP 3488A Switch/Control Unit: Operating, Programming, and
 *   Configuration Manual", Sept 1, 1995.
 *
 * Notes:
 * <slot> is slot number 1-5 (1 digit)
 * <c> is channel address <slot><chan> (3 digits)
 */


#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#if HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <assert.h>
#include <sys/time.h>
#include <math.h>

#include "libutil/util.h"
#include "libinst/inst.h"
#include "libinst/cmdline.h"
#include "libutil/argv.h"
#include "liblsd/hostlist.h"

#define INSTRUMENT "hp3488"

#define HP3488_DMODE_MODE_STATIC    1       /* default */
#define HP3488_DMODE_MODE_STATIC2   2       /* read what was written */
#define HP3488_DMODE_MODE_RWSTROBE  3       /* read & write strobe mode */
#define HP3488_DMODE_MODE_HANDSHAKE 4       /* handshake (no ext. inc. mode) */

#define HP3488_DMODE_POLARITY_LBP   0x01    /* lower byte polarity */
#define HP3488_DMODE_POLARITY_UBP   0x02    /* upper byte polarity */
#define HP3488_DMODE_POLARITY_PCTL  0x04    /* PCTL polarity (low ready) */
#define HP3488_DMODE_POLARITY_PFLG  0x08    /* PFLG polarity (low ready) */
#define HP3488_DMODE_POLARITY_IODIR 0x16    /* I/O direction line polarity */
                                            /*   high = input mode normally */

/* Status byte values
 * Also SRQ mask values (except rqs cannot be masked)
 */
#define HP3488_STATUS_SCAN_DONE     0x01    /* end of scan sequence */
#define HP3488_STATUS_OUTPUT_AVAIL  0x02    /* output available */
#define HP3488_STATUS_SRQ_POWER     0x04    /* power on SRQ asserted */
#define HP3488_STATUS_SRQ_BUTTON    0x08    /* front panel SRQ asserted */
#define HP3488_STATUS_READY         0x10    /* ready for instructions */
#define HP3488_STATUS_ERROR         0x20    /* an error has occurred */
#define HP3488_STATUS_RQS           0x40    /* rqs */

/* Error byte values
 */
#define HP3488_ERROR_SYNTAX         0x1     /* syntax error */
#define HP3488_ERROR_EXEC           0x2     /* exection error */
#define HP3488_ERROR_TOOFAST        0x4     /* hardware trigger too fast */
#define HP3488_ERROR_LOGIC          0x8     /* logic failure */
#define HP3488_ERROR_POWER          0x10    /* power supply failure */

/* Card model numbers, descriptions, and valid channel numbers.
 * Note: 44477 and 44476 are reported as 44471 by CTYPE command.
 */
#define MODELTAB    { \
    { 00000, "empty slot",            NULL }, \
    { 44470, "relay multiplexer",     "[00-09]" }, \
    { 44471, "general purpose relay", "[00-09]" }, \
    { 44472, "dual 4-1 VHF switch",   "[00-03,10-13]" }, \
    { 44473, "4x4 matrix switch",     "[00-03,10-13,20-23,30-33]" }, \
    { 44474, "digital I/O",           "[00-15]" }, \
    { 44475, "breadboard",            NULL }, \
    { 44476, "microwave switch",      "[00-03]" }, \
    { 44477, "form C relay",          "[00-06]" }, \
    { 44478, "1.3GHz multiplexer",    "[00-03,10-13]" }, \
}

static void _get_idn(struct instrument *gd);
static void _test(struct instrument *gd);
static void _clear(struct instrument *gd);
static void _shell(struct instrument *gd);
static void _show_config(void);
static void _open_targets(struct instrument *gd, char *str);
static void _close_targets(struct instrument *gd, char *str);
static void _query_targets(struct instrument *gd, char *str);
static void _list_targets();
static int _interpret_status(struct instrument *gd, unsigned char status, char *msg);
static int _parse_model_config(char *str);
static void _probe_model_config(struct instrument *gd);

typedef struct {
    int model;
    char *desc;
    char *list;
} modeltab_t;

static modeltab_t modeltab[] = MODELTAB;
static hostlist_t valid_targets = NULL;
static int slot_config[5] = { 0, 0, 0, 0, 0 };

char *prog = "";
static int lerr_flag = 1;   /* logic errors: 0=ignored, 1=fatal */
                            /*   See: disambiguate_ctype() */

static const char *options = OPTIONS_COMMON "licSL0:1:q:QIC:x";

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    OPTIONS_COMMON_LONG,
    {"get-idn",         no_argument,       0, 'i'},
    {"local",           no_argument,       0, 'l'},
    {"clear",           no_argument,       0, 'c'},
    {"selftest",        no_argument,       0, 'S'},
    {"list",            no_argument,       0, 'L'},
    {"showconfig",      no_argument,       0, 'x'},
    {"open",            required_argument, 0, '0'},
    {"close",           required_argument, 0, '1'},
    {"query",           required_argument, 0, 'q'},
    {"queryall",        no_argument,       0, 'Q'},
    {"config",          required_argument, 0, 'C'},
    {"shell",           no_argument,       0, 'I'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    { "l", "local", "return instrument to front panel control" },
    { "c", "clear", "initialize instrument to default values" },
    { "i", "get-idn", "get instrument idn string" },
    { "C", "config m,m,m,m,m", "specify model card in slots 1,2,3,4,5 where\n\
model is 44470|44471|44472|44473|44474|44475|44476|44477|44478 (0=empty)" },
    { "x", "showconfig", "list installed cards" },
    { "S", "selftest", "execute self test" },
    { "L", "list", "list valid channels" },
    { "0", "open [targets]", "open contacts for specified channels" },
    { "1", "close [targets]", "close contacts for specified channels" },
    { "q", "query [targets]", "view state of specified channels" },
    { "Q", "queryall", "view state of all channels" },
    { "I", "shell", "start interactive shell" },
    { 0, 0, 0 },
};

int
main(int argc, char *argv[])
{
    int need_valid_targets = 0;
    int c, print_usage = 0;
    int exit_val = 0;
    struct instrument *gd;

    gd = inst_init_args(argc, argv, options, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage) {
        usage(optdesc);
        exit(1);
    }
    if (!gd)
        exit(1);

    inst_set_reos(gd, 1);

    /* preparse args (again) to get slot config settled before doing work */
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            case 'C': /* --config */
                if (valid_targets) {
                    fprintf(stderr, "%s: -C may only be used once\n", prog);
                    exit_val = 1;
                    goto done;
                }
                valid_targets = hostlist_create("");
                if (_parse_model_config(optarg) == -1) {
                    exit_val = 1;
                    goto done;
                }
                break;
            case 'l': /* handled below */
            case 'c':
            case 'S':
            case 'i':
                break;
            case 'L':
            case '0':
            case '1':
            case 'q':
            case 'Q':
            case 'I':
            case 'x':
                need_valid_targets++;
                break;
        }
    }

    if (need_valid_targets && valid_targets == NULL) {
        valid_targets = hostlist_create("");
        _probe_model_config(gd);
    }

    optind = 0;
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH
                break;
            case 'C': /* --config (handled above) */
                break;
            case 'l': /* --local */
                inst_loc(gd);
                break;
            case 'c': /* --clear */
                _clear(gd);
                break;
            case 'i': /* --get-idn */
                _get_idn(gd);
                break;
            case 'S': /* --selftest */
                _test(gd);
                break;
            case 'L': /* --list */
                _list_targets();
                break;
            case '0': /* --open */
                _open_targets(gd, optarg);
                break;
            case '1': /* --close */
                _close_targets(gd, optarg);
                break;
            case 'q': /* --query */
                _query_targets(gd, optarg);
                break;
            case 'Q': /* --queryall */
                _query_targets(gd, NULL);
                break;
            case 'I': /* --shell */
                _shell(gd);
                break;
            case 'x': /* --showconfig */
                _show_config();
                break;
        }
    }

done:
    if (valid_targets)
        hostlist_destroy(valid_targets);
    inst_fini(gd);
    exit(exit_val);
}

/* Check the error register.
 *   Return: 0=nonfatal, 1=fatal.
 */
static int
_check_error(struct instrument *gd, char *msg)
{
    char tmpbuf[64];
    int res = 0;
    unsigned char err;

    inst_wrtstr(gd, "ERROR");
    inst_rdstr(gd, tmpbuf, sizeof(tmpbuf));   /* clears error */

    err = strtoul(tmpbuf, NULL, 10);
    if ((err & HP3488_ERROR_SYNTAX)) {
        fprintf(stderr, "%s: %s: syntax error\n", prog, msg);
        res = 1;
    }
    if ((err & HP3488_ERROR_EXEC)) {
        fprintf(stderr, "%s: %s: execution error\n", prog, msg);
        res = 1;
    }
    if ((err & HP3488_ERROR_TOOFAST)) {
        fprintf(stderr, "%s: %s: trigger too fast\n", prog, msg);
        res = 1;
    }
    if ((err & HP3488_ERROR_POWER)) {
        fprintf(stderr, "%s: %s: power supply failure\n", prog, msg);
        res = 1;
    }
    if ((err & HP3488_ERROR_LOGIC)) {
        if (lerr_flag == 0) {
            lerr_flag = 1;
        } else {
            fprintf(stderr, "%s: %s: logic error\n", prog, msg);
            res = 1;
        }
    }
    return res;
}

/* Interpet serial poll results (status byte)
 *   Return: 0=non-fatal/no error, >0=fatal, -1=retry.
 */
static int
_interpret_status(struct instrument *gd, unsigned char status, char *msg)
{
    int err = 0;

    if ((status & HP3488_STATUS_SRQ_POWER)) {
        fprintf(stderr, "%s: %s: power-on SRQ occurred\n", prog, msg);
    }
    if ((status & HP3488_STATUS_SRQ_BUTTON)) {
        fprintf(stderr, "%s: %s: front panel SRQ key pressed\n", prog, msg);
        err = 1;
    }
    if ((status & HP3488_STATUS_RQS)) {
        fprintf(stderr, "%s: %s: rqs\n", prog, msg);
        err = 1;
    }
    if ((status & HP3488_STATUS_ERROR)) {
        err = _check_error(gd, msg);
    }
    if (err == 0 && !(status & HP3488_STATUS_READY))
        err = -1;

    return err;
}


/* Close a couple of relays to disambiguate 44477 and 44476 from 44471.
 * Helper for _probe_model_config().
 */
static int
_disambiguate_ctype(struct instrument *gd, int slot)
{
    int err4, err9;
    int model;

    inst_wrtf(gd, "DOFF");

    fprintf(stderr, "%s: closing switches in slot %d to disambiguate card type\n", prog, slot);
    fprintf(stderr, "%s: avoid this in the future by using -C option\n", prog);

    lerr_flag = 0;  /* next logic error will set lerr_flag */
    inst_wrtf(gd, "CLOSE %d04", slot);
    err4 = lerr_flag;
    lerr_flag = 0;  /* next logic error will set lerr_flag */
    inst_wrtf(gd, "CLOSE %d09", slot);
    err9 = lerr_flag;
    lerr_flag = 1;  /* back to default logic error handling */

    if (err4 && err9)
        model = 44476;
    else if (!err4 && err9)
        model = 44477;
    else if (!err4 && !err9)
        model = 44471;
    else {
        fprintf(stderr, "_disambiguate_ctype failed\n");
        exit(1);
    }

    inst_wrtf(gd, "CRESET %d", slot);
    inst_wrtf(gd, "DON");

    return model;
}

/* Return model number of option plugged into the specified slot.
 * Warning: 44477 and 44476 will look like 44471.
 */
static int
_ctype(struct instrument *gd, int slot)
{
    char buf[128];

    inst_wrtf(gd, "CTYPE %d", slot);
    inst_rdstr(gd, buf, sizeof(buf));

    if (strlen(buf) < 15) {
        fprintf(stderr, "%s: parse error reading slot ctype\n", prog);
        exit(1);
    }
    return strtoul(buf+11, NULL, 10);
}

static modeltab_t *
modeltab_find(int model)
{
    int i;

    for (i = 0; i < sizeof(modeltab)/sizeof(modeltab_t); i++)
        if (modeltab[i].model == model)
            return &modeltab[i];
    return NULL;
}

/* Set slot config by command line option.
 */
static int
_parse_model_config(char *str)
{
    char *modstr;
    int slot;
    char tmpstr[128];
    modeltab_t *cp;
    int model;

    for (slot = 1; slot <= 5; slot++) {
        modstr = strtok(slot == 1 ? str : NULL, ",");
        if (!modstr) {
            fprintf(stderr, "%s: specify models for all five slots (0=empty)\n",
                    prog);
            return -1;
        }
        model = strtoul(modstr, NULL, 10);
        if (!(cp = modeltab_find(model))) {
            fprintf(stderr, "%s: unknown model: %s\n", prog, modstr);
            return -1;
        }
        if (cp->list) {
            sprintf(tmpstr, "%d%s", slot, cp->list);
            hostlist_push(valid_targets, tmpstr);
        }
        slot_config[slot - 1] = model;
    }
    hostlist_sort(valid_targets);
    return 0;
}

static void
_probe_model_config(struct instrument *gd)
{
    int slot, model;
    modeltab_t *cp;
    char tmpstr[128];

    for (slot = 1; slot <= 5; slot++) {
        model = _ctype(gd, slot);
        if (model == 44471) {
            /* XXX issue a warning here and default to not closing switches? */
            model = _disambiguate_ctype(gd, slot);
        }
        cp = modeltab_find(model);
        if (cp->list) {
            sprintf(tmpstr, "%d%s", slot, cp->list);
            hostlist_push(valid_targets, tmpstr);
        }
        slot_config[slot - 1] = model;
    }
    hostlist_sort(valid_targets);
}

static void
query_caddr(struct instrument *gd, char *caddr)
{
    char buf[128];

    inst_wrtf(gd, "VIEW %s", caddr);
    inst_rdstr(gd, buf, sizeof(buf));

    if (!strncmp(buf, "OPEN   1", 8))
        printf("%s: 0\n", caddr);
    else if (!strncmp(buf, "CLOSED 0", 8))
        printf("%s: 1\n", caddr);
    else
        printf("%s: parse error \n", caddr);
}

static void
_show_config(void)
{
    int i;
    modeltab_t *cp;

    for (i = 1; i <= 5; i++) {
        cp = modeltab_find(slot_config[i - 1]);
        assert(cp != NULL);
        printf("%d: %d %-30.30s %s\n", i, cp->model, cp->desc,
            cp->list ? cp->list : "");
    }
}

/* For some reason hostlist.c::hostlist_find() is not working for me... */
static int
my_hostlist_find(hostlist_t hl, char *key)
{
    hostlist_iterator_t it;
    char *str;
    int res = -1;

    it = hostlist_iterator_create(hl);
    while ((str = hostlist_next(it))) {
        if (!strcmp(str, key)) {
            res = 0;
            break;
        }
    }
    hostlist_iterator_destroy(it);
    return res;
}

static void
_query_targets(struct instrument *gd, char *str)
{
    hostlist_iterator_t it;
    hostlist_t hl = NULL;
    char *caddr;

    if (str)
        hl = hostlist_create(str);
    it = hostlist_iterator_create(hl ? hl : valid_targets);
    while ((caddr = hostlist_next(it))) {
        if (my_hostlist_find(valid_targets, caddr) == -1)
            fprintf(stderr, "%s: %s: invalid channel address\n", prog, caddr);
        else
            query_caddr(gd, caddr);
    }
    hostlist_iterator_destroy(it);
    if (hl)
        hostlist_destroy(hl);
}

static void
_open_targets(struct instrument *gd, char *str)
{
    hostlist_iterator_t it;
    hostlist_t hl;
    char *caddr;

    hl = hostlist_create(str);
    it = hostlist_iterator_create(hl);
    while ((caddr = hostlist_next(it))) {
        if (my_hostlist_find(valid_targets, caddr) == -1)
            fprintf(stderr, "%s: %s: invalid channel address\n", prog, caddr);
        else
            inst_wrtf(gd, "OPEN %s", caddr);
    }
    hostlist_iterator_destroy(it);
    hostlist_destroy(hl);
}

static void
_close_targets(struct instrument *gd, char *str)
{
    hostlist_iterator_t it;
    hostlist_t hl;
    char *caddr;

    hl = hostlist_create(str);
    it = hostlist_iterator_create(hl);
    while ((caddr = hostlist_next(it))) {
        if (my_hostlist_find(valid_targets, caddr) == -1)
            fprintf(stderr, "%s: %s: invalid channel address\n", prog, caddr);
        else
            inst_wrtf(gd, "CLOSE %s", caddr);
    }
    hostlist_iterator_destroy(it);
    hostlist_destroy(hl);
}

static void
_list_targets()
{
    hostlist_iterator_t it;
    char *caddr;

    it = hostlist_iterator_create(valid_targets);
    while ((caddr = hostlist_next(it)))
        printf("%s\n", caddr);
    hostlist_iterator_destroy(it);
}

/* Run the instrument self test.
 */
static void
_test(struct instrument *gd)
{
    inst_wrtf(gd, "TEST");
    /* if we don't exit via serial poll callback, we passed */
    printf("%s: selftest passed\n", prog);
}

/* Clear instrument to default settings: relays open, dig I/O to hi-Z.
 * Stored setups not destroyed.
 * Verify identity.
 */
static void
_clear(struct instrument *gd)
{
    char tmpbuf[64];

    inst_clr(gd, 1000000);

    /* verify the instrument id is what we expect */
    inst_wrtf(gd, "ID?");
    inst_rdstr(gd, tmpbuf, sizeof(tmpbuf));

    if (strncmp(tmpbuf, "HP3488A", 7) != 0) {
        fprintf(stderr, "%s: device id %s != %s\n", prog, tmpbuf, "HP3488A");
        exit(1);
    }
}

static void
_shell_help(void)
{
    printf("Possible commands are:\n");
    printf("   on [targets]    - turn on specified channels\n");
    printf("   off [targets]   - turn off specified channels\n");
    printf("   query [targets] - query specified channels (0=off,1=on)\n");
    printf("   list            - list valid caddrs\n");
    printf("   show            - show slot configuration\n");
    printf("   id              - query instrument id\n");
    printf("   verbose         - toggle verbose flag\n");
    printf("   help            - display this help text\n");
    printf("   quit            - exit this shell\n");
    printf("Where channel address is <slot><chan> (3 digits).\n");
}

static int
_docmd(struct instrument *gd, char **av)
{
    int rc = 0;

    if (av[0]) {
        if (strcmp(av[0], "help") == 0) {
            _shell_help();

        } else if (strcmp(av[0], "quit") == 0) {
            rc = 1;

        } else if (strcmp(av[0], "on") == 0) {
            if (av[1] == NULL)
                printf("Usage: on [targets]\n");
            else
                _close_targets(gd, av[1]);

        } else if (strcmp(av[0], "off") == 0) {
            if (av[1] == NULL)
                printf("Usage: off [targets]\n");
            else
                _open_targets(gd, av[1]);

        } else if (strcmp(av[0], "query") == 0) {
            _query_targets(gd, av[1]);

        } else if (strcmp(av[0], "show") == 0) {
            _show_config();

        } else if (strcmp(av[0], "list") == 0) {
            if (av[1] != NULL)
                printf("Usage: list\n");
            else
                _list_targets();

        } else if (strcmp(av[0], "id") == 0) {
            _get_idn(gd);
        } else if (strcmp(av[0], "verbose") == 0) {
            if (av[1] == NULL || av[2] != NULL)
                printf("Usage: verbose 1|0\n");
            else {
                if (!strcmp(av[1], "1"))
                    inst_set_verbose(gd, 1);
                else if (!strcmp(av[1], "0"))
                    inst_set_verbose(gd, 0);
                else
                    printf("Usage: verbose 1|0\n");
            }

        } else
            printf("type \"help\" for a list of commands\n");
    }
    return rc;
}

static void
_shell(struct instrument *gd)
{
    char buf[128];
    char **av;
    int rc = 0;

    while (rc == 0 && (xreadline("hp3488> ", buf, sizeof(buf)))) {
        if (strlen(buf) > 0) {
            av = argv_create(buf, "");
            rc = _docmd(gd, av);
            argv_destroy(av);
        }
    }
}

static void
_get_idn(struct instrument *gd)
{
    char model[64];

    inst_wrtstr(gd, "ID?\n");
    inst_rdstr(gd, model, sizeof(model));
    printf("%s\n", model);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
