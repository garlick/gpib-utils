/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005 Jim Garlick <garlick@speakeasy.net>

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
#if HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "hp3488.h"
#include "gpib.h"
#include "util.h"
#include "argv.h"
#include "hostlist.h"

#define INSTRUMENT "hp3488"

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

#define OPTIONS "a:clSvL0:1:q:QIC:x"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
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

static void 
usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -a,--address                  instrument address [%s]\n"
"  -c,--clear                    initialize instrument to default values\n"
"  -l,--local                    return instrument to local operation on exit\n"
"  -v,--verbose                  show protocol on stderr\n"
"  -S,--selftest                 execute self test\n"
"  -L,--list                     list valid channels\n"
"  -0,--open [targets]           open contacts for specified channels\n"
"  -1,--close [targets]          close contacts for specified channels\n"
"  -q,--query [targets]          view state of specified channels\n"
"  -Q,--queryall                 view state of all channels\n"
"  -I,--shell                    start interactive shell\n"
"  -x,--showconfig               list installed cards\n"
"  -C,--config m,m,m,m,m         specify model card in slots 1,2,3,4,5 where\n"
"    model is 44470|44471|44472|44473|44474|44475|44476|44477|44478 (0=empty)\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

/* Check the error register.
 *   Return: 0=nonfatal, 1=fatal.
 */
static int
_check_error(gd_t gd, char *msg)
{
    char tmpbuf[64];
    int res = 0;
    unsigned char err;

    gpib_wrtstr(gd, HP3488_ERR_QUERY);
    gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));   /* clears error */

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
_interpret_status(gd_t gd, unsigned char status, char *msg)
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
 * Helper for probe_model_config().
 */
static int
_disambiguate_ctype(gd_t gd, int slot)
{
    int err4, err9;
    int model;

    gpib_wrtf(gd, "%s", HP3488_DISP_OFF);

    fprintf(stderr, "%s: closing switches in slot %d to disambiguate card type\n", prog, slot);
    fprintf(stderr, "%s: avoid this in the future by using -C option\n", prog);

    lerr_flag = 0;  /* next logic error will set lerr_flag */
    gpib_wrtf(gd, "%s %d04", HP3488_CLOSE, slot);
    err4 = lerr_flag;
    lerr_flag = 0;  /* next logic error will set lerr_flag */
    gpib_wrtf(gd, "%s %d09", HP3488_CLOSE, slot);
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

    gpib_wrtf(gd, "%s %d", HP3488_CRESET, slot);
    gpib_wrtf(gd, "%s", HP3488_DISP_ON);

    return model;
}

/* Return model number of option plugged into the specified slot.
 * Warning: 44477 and 44476 will look like 44471.
 */
static int
_ctype(gd_t gd, int slot)
{
    char buf[128];

    gpib_wrtf(gd, "%s %d", HP3488_CTYPE, slot);
    gpib_rdstr(gd, buf, sizeof(buf));

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
parse_model_config(char *str)
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
probe_model_config(gd_t gd)
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
query_caddr(gd_t gd, char *caddr)
{
    char buf[128];

    gpib_wrtf(gd, "%s %s", HP3488_VIEW_QUERY, caddr);
    gpib_rdstr(gd, buf, sizeof(buf));

    if (!strncmp(buf, "OPEN   1", 8))
        printf("%s: 0\n", caddr);
    else if (!strncmp(buf, "CLOSED 0", 8))
        printf("%s: 1\n", caddr);
    else
        printf("%s: parse error \n", caddr);
}

static void
show_config(void)
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
query_targets(gd_t gd, char *str)
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
open_targets(gd_t gd, char *str)
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
            gpib_wrtf(gd, "%s %s", HP3488_OPEN, caddr);
    }
    hostlist_iterator_destroy(it);
    hostlist_destroy(hl);
}

static void
close_targets(gd_t gd, char *str)
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
            gpib_wrtf(gd, "%s %s", HP3488_CLOSE, caddr);
    }
    hostlist_iterator_destroy(it);
    hostlist_destroy(hl);
}

static void
list_targets()
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
instrument_test(gd_t gd)
{
    gpib_wrtf(gd, "%s", HP3488_TEST);
    /* if we don't exit via serial poll callback, we passed */
    printf("%s: selftest passed\n", prog);
}

/* Clear instrument to default settings: relays open, dig I/O to hi-Z.
 * Stored setups not destroyed.
 * Verify identity.
 */
static void
instrument_clear(gd_t gd)
{
    char tmpbuf[64];

    gpib_clr(gd, 1000000);

    /* verify the instrument id is what we expect */
    gpib_wrtf(gd, "%s", HP3488_ID_QUERY);
    gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));

    if (strncmp(tmpbuf, HP3488_ID_RESPONSE, strlen(HP3488_ID_RESPONSE)) != 0) {
        fprintf(stderr, "%s: device id %s != %s\n", prog, tmpbuf,
                HP3488_ID_RESPONSE);
        exit(1);
    }
}

#if HAVE_LIBREADLINE
static void 
shell_help(void)
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
docmd(gd_t gd, char **av)
{
    int rc = 0;

    if (av[0]) {
        if (strcmp(av[0], "help") == 0) {
            shell_help();

        } else if (strcmp(av[0], "quit") == 0) {
            rc = 1;

        } else if (strcmp(av[0], "on") == 0) {
            if (av[1] == NULL)
                printf("Usage: on [targets]\n");
            else
                close_targets(gd, av[1]);

        } else if (strcmp(av[0], "off") == 0) {
            if (av[1] == NULL)
                printf("Usage: off [targets]\n");
            else
                open_targets(gd, av[1]);

        } else if (strcmp(av[0], "query") == 0) {
            query_targets(gd, av[1]);

        } else if (strcmp(av[0], "show") == 0) {
            show_config();

        } else if (strcmp(av[0], "list") == 0) {
            if (av[1] != NULL)
                printf("Usage: list\n");
            else
                list_targets();

        } else if (strcmp(av[0], "id") == 0) {
            char tmpbuf[128];

            gpib_wrtf(gd, "%s", HP3488_ID_QUERY);
            gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));
            printf("%s\n", tmpbuf);

        } else if (strcmp(av[0], "verbose") == 0) {
            if (av[1] == NULL || av[2] != NULL)
                printf("Usage: verbose 1|0\n");
            else {
                if (!strcmp(av[1], "1"))
                    gpib_set_verbose(gd, 1);
                else if (!strcmp(av[1], "0"))
                    gpib_set_verbose(gd, 0);
                else
                    printf("Usage: verbose 1|0\n");
            }

        } else
            printf("type \"help\" for a list of commands\n");
    }
    return rc;
}

static void
shell_interact(gd_t gd)
{
    char *line;
    char **av;
    int rc = 0;

    /* disable readline file name completion */
    /*rl_bind_key ('\t', rl_insert); */

    while (rc == 0 && (line = readline("hp3488> "))) {
        if (strlen(line) > 0) {
            add_history(line);
            av = argv_create(line, "");
            rc = docmd(gd, av);
            argv_destroy(av);
        } 
        free(line);
    }
}
#else
static void
shell_interact(gd_t gd)
{
    fprintf(stderr, "%s: not configured with readline support\n", prog);
}
#endif /* HAVE_LIBREADLINE */

int
main(int argc, char *argv[])
{
    char *addr = NULL;
    gd_t gd;
    int c;
    int verbose = 0;
    int clear = 0;
    int local = 0;
    int list = 0;
    int selftest = 0;
    int shell = 0;
    int todo = 0;
    int open = 0;
    int close = 0;
    int query = 0;
    int showconfig = 0;
    char *targets = NULL;
    int need_valid_targets = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
        case 'a': /* --address */
            addr = optarg;
            break;
        case 'c': /* --clear */
            clear = 1;
            todo++;
            break;
        case 'l': /* --local */
            local = 1;
            todo++;
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        case 'L': /* --list */
            list = 1;
            need_valid_targets++;
            todo++;
            break;
        case '0': /* --open */
            open = 1;
            targets = optarg;
            need_valid_targets++;
            todo++;
            break;
        case '1': /* --close */
            close = 1;
            targets = optarg;
            need_valid_targets++;
            todo++;
            break;
        case 'q': /* --query */
            query = 1;
            targets = optarg;
            need_valid_targets++;
            todo++;
            break;
        case 'Q': /* --queryall */
            query = 1;
            targets = NULL;
            need_valid_targets++;
            todo++;
            break;
        case 'S': /* --selftest */
            selftest = 1;
            todo++;
            break;
        case 'I': /* --shell */
            shell = 1;
            need_valid_targets++;
            todo++;
            break;
        case 'C': /* --config */
            if (valid_targets) {
                fprintf(stderr, "%s: -C may only be specified once\n", prog);
                exit(1);
            }
            valid_targets = hostlist_create("");
            if (parse_model_config(optarg) == -1)
                exit(1);
            break;
        case 'x': /* --showconfig */
            showconfig = 1;
            todo++;
            need_valid_targets++;
            break;
        default:
            usage();
            break;
        }
    }
    if (optind < argc)
        usage();
    if (!todo)
        usage();

    if (!addr)
        addr = gpib_default_addr(INSTRUMENT);
    if (!addr) {
        fprintf(stderr, "%s: no default address for %s, use --address\n",
                prog, INSTRUMENT);
        exit(1);
    }
    gd = gpib_init(addr , _interpret_status, 100000);
    if (!gd) {
        fprintf(stderr, "%s: device initialization failed for address %s\n", 
                prog, addr);
        exit(1);
    }
    gpib_set_verbose(gd, verbose);
    gpib_set_reos(gd, 1);

    if (clear)
        instrument_clear(gd);

    if (selftest)
        instrument_test(gd);

    if (need_valid_targets && valid_targets == NULL) {
        valid_targets = hostlist_create("");
        probe_model_config(gd);
    }

    if (showconfig)
        show_config();
    if (list)
        list_targets();
    if (open)
        open_targets(gd, targets);
    if (close)
        close_targets(gd, targets);
    if (query)
        query_targets(gd, targets);
    if (shell)
        shell_interact(gd);

    if (local)
        gpib_loc(gd); 

    if (valid_targets)
        hostlist_destroy(valid_targets);

    gpib_fini(gd);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
