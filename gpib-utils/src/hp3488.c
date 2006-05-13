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

#define _GNU_SOURCE /* for asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>

#include "hp3488.h"
#include "gpib.h"

#define INSTRUMENT "hp3488" /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

static char *prog = "";

#define OPTIONS "n:cltvL0:1:q:Q"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"selftest",        no_argument,       0, 't'},
    {"list",            no_argument,       0, 'L'},
    {"open",            required_argument, 0, '0'},
    {"close",           required_argument, 0, '1'},
    {"query",           required_argument, 0, 'q'},
    {"queryall",        no_argument,       0, 'Q'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name                instrument name [%s]\n"
"  -c,--clear                    initialize instrument to default values\n"
"  -l,--local                    return instrument to local operation on exit\n"
"  -v,--verbose                  show protocol on stderr\n"
"  -0,--open caddr[,caddr]...    open contacts for specified channels\n"
"  -1,--close caddr[,caddr]...   close contacts for specified channels\n"
"  -q,--query caddr[,caddr]...   view state of specified channels (or slots)\n"
"  -Q,--queryall                 view state of all channels\n"
"  -t,--selftest                 execute self test\n"
"  -L,--list                     list slot configuration (may alter state!)\n"
           , prog, INSTRUMENT);
    exit(1);
}

static int
_checkerror(int d)
{
    char tmpbuf[64];

    gpib_ibwrtstr(d, HP3488_ERR_QUERY);
    gpib_ibrdstr(d, tmpbuf, sizeof(tmpbuf)); /* clears error */

    return strtoul(tmpbuf, NULL, 10);
}

static void
_reporterror(int err)
{
    /* presumably since error is a mask, we can have more than one? */
    if ((err & HP3488_ERROR_SYNTAX))
        fprintf(stderr, "%s: syntax error\n", prog);
    if ((err & HP3488_ERROR_EXEC))
        fprintf(stderr, "%s: execution error\n", prog);
    if ((err & HP3488_ERROR_TOOFAST))
        fprintf(stderr, "%s: trigger too fast\n", prog);
    if ((err & HP3488_ERROR_LOGIC))
        fprintf(stderr, "%s: logic error\n", prog);
    if ((err & HP3488_ERROR_POWER))
        fprintf(stderr, "%s: power supply failure\n", prog);
}

/* Spin until ready for another command or an error has occurred.
 * FIXME: we should go to sleep and let device driver wake us up.
 */
static void
hp3488_checksrq(int d, int *errp)
{
    char status;
    int ready = 0;
    int err = 0;
    int loops = 0;
    int fatal = 0;

    do {
        usleep(loops*100000); /* backoff-delay */
        gpib_ibrsp(d, &status);
        if ((status & HP3488_STATUS_READY)) {
            ready = 1;
        }
        if ((status & HP3488_STATUS_SRQ_POWER)) {
            fprintf(stderr, "%s: srq: power-on SRQ occurred\n", prog);
            fatal = 1;
        }
        if ((status & HP3488_STATUS_SRQ_BUTTON)) {
            fprintf(stderr, "%s: srq: front panel SRQ key pressed\n", prog);
            fatal = 1;
        }
        if ((status & HP3488_STATUS_ERROR)) {
            err = _checkerror(d); /* clears error */
        }
        if ((status & HP3488_STATUS_RQS)) {
            fprintf(stderr, "%s: srq: rqs\n", prog);
            fatal = 1;
        }
        loops++;
    } while (!ready && !err && !fatal);

    if (errp)
        *errp = err;
    if (!errp && err) {
        _reporterror(err);
        fatal = 1;
    }
    if (fatal)
        exit(1);
}

static void
hp3488_checkid(int d)
{
    char tmpbuf[64];

    gpib_ibwrtf(d, "%s", HP3488_ID_QUERY);
    hp3488_checksrq(d, NULL);

    gpib_ibrdstr(d, tmpbuf, sizeof(tmpbuf));
    hp3488_checksrq(d, NULL);

    if (strncmp(tmpbuf, HP3488_ID_RESPONSE, strlen(HP3488_ID_RESPONSE)) != 0) {
        fprintf(stderr, "%s: device id %s != %s\n", prog, tmpbuf,
                HP3488_ID_RESPONSE);
        exit(1);
    }
}

static void
hp3488_open(int d, char *list)
{
    gpib_ibwrtf(d, "%s %s", HP3488_OPEN, list);
    hp3488_checksrq(d, NULL);
}

static void
hp3488_close(int d, char *list)
{
    gpib_ibwrtf(d, "%s %s", HP3488_CLOSE, list);
    hp3488_checksrq(d, NULL);
}


/* Return model number of option plugged into the specified slot.
 * Warning: 44477 and 44476 will look like 44471.
 */
static int
hp3488_ctype(int d, int slot)
{
    char buf[128];

    gpib_ibwrtf(d, "%s %d", HP3488_CTYPE, slot);
    hp3488_checksrq(d, NULL);

    gpib_ibrdstr(d, buf, sizeof(buf));
    hp3488_checksrq(d, NULL);

    if (strlen(buf) < 15) {
        fprintf(stderr, "%s: parse error reading slot ctype\n", prog);
        exit(1);
    }
    return strtoul(buf+11, NULL, 10);
}

/* Close a couple of relays to disambiguate 44477 and 44476 from 44471.
 */
static int
_disambiguate_ctype(int d, int slot)
{
    int err4, err9;
    int model;

    gpib_ibwrtf(d, "%s", HP3488_DISP_OFF);
    hp3488_checksrq(d, NULL);

    gpib_ibwrtf(d, "%s %d04", HP3488_CLOSE, slot);
    hp3488_checksrq(d, &err4);
    if (!(err4 == 0 || err4 == HP3488_ERROR_LOGIC)) {
        _reporterror(err4);
        exit(1);
    }

    gpib_ibwrtf(d, "%s %d09", HP3488_CLOSE, slot);
    hp3488_checksrq(d, &err9);
    if (!(err9 == 0 || err9 == HP3488_ERROR_LOGIC)) {
        _reporterror(err9);
        exit(1);
    }

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

    gpib_ibwrtf(d, "%s %d", HP3488_CRESET, slot);
    hp3488_checksrq(d, NULL);

    gpib_ibwrtf(d, "%s", HP3488_DISP_ON);
    hp3488_checksrq(d, NULL);

    return model;
}

typedef struct {
    int model;
    char *desc;
    char *list;
} caddrtab_t;

static caddrtab_t ctab[] = CADDRTAB;

static caddrtab_t *
_caddrtab_find(int model)
{
    int i;

    for (i = 0; i < sizeof(ctab)/sizeof(caddrtab_t); i++)
        if (ctab[i].model == model)
            return &ctab[i];
    return NULL;
}

static void
hp3488_query_caddr(int d, char *caddr)
{
    char buf[128];
    int state;

    gpib_ibwrtf(d, "%s %s", HP3488_VIEW_QUERY, caddr);
    hp3488_checksrq(d, NULL);
               
    gpib_ibrdstr(d, buf, sizeof(buf));
    hp3488_checksrq(d, NULL);

    if (!strncmp(buf, "OPEN   1", 8))
        state = 0;
    else if (!strncmp(buf, "CLOSED 0", 8))
        state = 1;
    else {
        fprintf(stderr, "%s: parse error\n", prog);
        exit(1);
    }
    printf("%s: %d\n", caddr, state);
}

static void
hp3488_query_slot(int d, int slot)
{
    int model;
    char *listcpy;
    caddrtab_t *cp;
    char *chan;
    char caddr[64];
    char buf[256];

    model = hp3488_ctype(d, slot);
    cp = _caddrtab_find(model);
    if (cp == NULL) {
        fprintf(stderr, "%s: problem looking up slot ctype %d\n", prog, model);
        exit(1);
    }
    if (cp->list) {
        listcpy = strdup(cp->list);
        if (listcpy == NULL) {
            fprintf(stderr, "%s: out of memory\n", prog);
            exit(1);
        }
        chan = strtok_r(listcpy, ",", (char **)&buf);
        while (chan) {
            sprintf(caddr, "%d%s", slot, chan);
            hp3488_query_caddr(d, caddr);
            chan = strtok_r(NULL, ",", (char **)&buf);
        }
        free(listcpy);
    }
}

static void
hp3488_query_all(int d)
{
    int slot;

    for (slot = 1; slot <= 5; slot++)
        hp3488_query_slot(d, slot);
}

static void
hp3488_query(int d, char *list)
{
    char *caddr;
    int slot;
    char buf[256];

    if (!list || strlen(list) == 0)  {
        hp3488_query_all(d);                    /* query all slots */
    } else {
        caddr = strtok_r(list, ",", (char **)&buf);
        while (caddr) {
            slot = strtoul(caddr, NULL, 10);
            if (slot >= 1 && slot <= 5)
                hp3488_query_slot(d, slot);     /* query whole slot */
            else
                hp3488_query_caddr(d, caddr);   /* query individual caddr */

            caddr = strtok_r(NULL, ",", (char **)&buf);
        }
    }
}

static void
hp3488_list(int d)
{
    int slot, model;
    caddrtab_t *cp;

    for (slot = 1; slot <= 5; slot++) {
        model = hp3488_ctype(d, slot);
        if (model == 44471)
            model = _disambiguate_ctype(d, slot);
        cp = _caddrtab_find(model);
        printf("%d: %-.5d %-21s %s\n", slot, model, cp->desc, 
                cp->list ? cp->list : "");
    }
}

static void
hp3488_test(int d)
{
    gpib_ibwrtf(d, "%s", HP3488_TEST);
    hp3488_checksrq(d, NULL);
    printf("%s: selftest passed\n", prog);
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int c, d;
    int verbose = 0;
    int clear = 0;
    int local = 0;
    int list = 0;
    char *open = NULL;
    char *close = NULL;
    char *query = NULL;
    int query_all = 0;
    int selftest = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'n': /* --name */
            instrument = optarg;
            break;
        case 'c': /* --clear */
            clear = 1;
            break;
        case 'l': /* --local */
            local = 1;
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        case 'L': /* --list */
            list = 1;
            break;
        case '0': /* --open */
            open = optarg;
            break;
        case '1': /* --close */
            close = optarg;
            break;
        case 'q': /* --query */
            query = optarg;
            break;
        case 'Q': /* --queryall */
            query_all = 1;
            break;
        case 't': /* --selftest */
            selftest = 1;
            break;
        default:
            usage();
            break;
        }
    }

    if (!clear && !local && !list && !open && !close && !query && !query_all
            && !selftest)
        usage();

    d = gpib_init(prog, instrument, verbose);

    /* Clear instrument to default settings: relays open, dig I/O to hi-Z.
     * Stored setups not destroyed.
     * Verify identity.
     */
    if (clear) {
        gpib_ibclr(d);
        sleep(1); 
        hp3488_checkid(d);
    }

    if (selftest)
        hp3488_test(d);

    if (list)
        hp3488_list(d);
    if (open)
        hp3488_open(d, open);
    if (close)
        hp3488_close(d, close);
    if (query)
        hp3488_query(d, query);
    if (query_all)
        hp3488_query(d, NULL);

    /* return front panel if requested */
    if (local)
        gpib_ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
