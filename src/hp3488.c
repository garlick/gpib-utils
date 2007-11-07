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

char *prog = "";
static int lerr_flag = 1;   /* logic errors: 0=ignored, 1=fatal */
                            /*   See: disambiguate_ctype() */

#define OPTIONS "n:clSvL0:1:q:Q"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"selftest",        no_argument,       0, 'S'},
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
"  -S,--selftest                 execute self test\n"
"  -Q,--queryall                 view state of all channels\n"
"  -L,--list                     list slot configuration (may alter state!)\n"
"  -0,--open caddr[,caddr]...    open contacts for specified channels\n"
"  -1,--close caddr[,caddr]...   close contacts for specified channels\n"
"  -q,--query caddr[,caddr]...   view state of specified channels (or slots)\n"
           , prog, INSTRUMENT);
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

/* Close a couple of relays to disambiguate 44477 and 44476 from 44471.
 */
static int
_disambiguate_ctype(gd_t gd, int slot)
{
    int err4, err9;
    int model;

    gpib_wrtf(gd, "%s", HP3488_DISP_OFF);

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

/* List the state of a particular channel address.
 */
static void
_query_caddr(gd_t gd, char *caddr)
{
    char buf[128];
    int state;

    gpib_wrtf(gd, "%s %s", HP3488_VIEW_QUERY, caddr);
    gpib_rdstr(gd, buf, sizeof(buf));

    if (!strncmp(buf,      "OPEN   1", 8))
        state = 0;
    else if (!strncmp(buf, "CLOSED 0", 8))
        state = 1;
    else {
        fprintf(stderr, "%s: parse error\n", prog);
        exit(1);
    }
    printf("%s: %d\n", caddr, state);
}

/* List the state of all channels in a particular slot.
 */
static void
_query_slot(gd_t gd, int slot)
{
    int model;
    char *listcpy;
    caddrtab_t *cp;
    char *chan;
    char caddr[64];
    char buf[256];

    model = _ctype(gd, slot);
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
            _query_caddr(gd, caddr);
            chan = strtok_r(NULL, ",", (char **)&buf);
        }
        free(listcpy);
    }
}

/* List the state of a list of channel addresses, or if list is NULL,
 * of every channel in the system. 
 */
static void
hp3488_query(gd_t gd, char *list)
{
    char *caddr;
    int slot;
    char buf[256];

    if (!list) {
        for (slot = 1; slot <= 5; slot++)       /* query all slots */
            _query_slot(gd, slot);
    } else {
        caddr = strtok_r(list, ",", (char **)&buf);
        while (caddr) {
            slot = strtoul(caddr, NULL, 10);
            if (slot >= 1 && slot <= 5)
                _query_slot(gd, slot);     /* query whole slot */
            else
                _query_caddr(gd, caddr);   /* query individual caddr */

            caddr = strtok_r(NULL, ",", (char **)&buf);
        }
    }
}

/* List the model numbers of card plugged into the mainframe.
 */
static void
hp3488_list(gd_t gd)
{
    int slot, model;
    caddrtab_t *cp;

    for (slot = 1; slot <= 5; slot++) {
        model = _ctype(gd, slot);
        if (model == 44471) {
            model = _disambiguate_ctype(gd, slot);
        }
        cp = _caddrtab_find(model);
        printf("%d: %-.5d %-21s %s\n", slot, model, cp->desc, 
                cp->list ? cp->list : "");
    }
}

static void
hp3488_open(gd_t gd, char *list)
{
    gpib_wrtf(gd, "%s %s", HP3488_OPEN, list);
}

static void
hp3488_close(gd_t gd, char *list)
{
    gpib_wrtf(gd, "%s %s", HP3488_CLOSE, list);
}

/* Run the instrument self test.
 */
static void
hp3488_test(gd_t gd)
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
hp3488_clear(gd_t gd)
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

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    gd_t gd;
    int c;
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
        case 'S': /* --selftest */
            selftest = 1;
            break;
        default:
            usage();
            break;
        }
    }

    if (optind < argc)
        usage();

    if (!clear && !local && !list && !open && !close && !query && !query_all
            && !selftest)
        usage();

    /* After every operation, perform serial poll, then call
     * _interpret_status() with the status byte as an argument.
     */
    gd = gpib_init(instrument, _interpret_status, 100000);
    if (!gd) {
        fprintf(stderr, "%s: couldn't find device %s in /etc/gpib.conf\n",                 prog, instrument);
        exit(1);
    }
    gpib_set_verbose(gd, verbose);

    if (clear)
        hp3488_clear(gd);

    if (selftest)
        hp3488_test(gd);
    if (list)
        hp3488_list(gd);
    if (open)
        hp3488_open(gd, open);
    if (close)
        hp3488_close(gd, close);
    if (query)
        hp3488_query(gd, query);
    if (query_all)
        hp3488_query(gd, NULL);

    if (local)
        gpib_loc(gd); 

    gpib_fini(gd);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
