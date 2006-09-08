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
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>
#include <stdint.h>

#include "sr620.h"
#include "units.h"
#include "gpib.h"
#include "util.h"

#define INSTRUMENT "sr620"  /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

#define MAXCONFBUF  256
#define MAXIDBUF    80

static char *prog = "";
static int verbose = 0;

#define OPTIONS "n:clvsr"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"save",            no_argument,       0, 's'},
    {"restore",         no_argument,       0, 'r'},
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
"  -s,--save                     save complete instrument setup to stdout\n"
"  -r,--restore                  restore instrument setup from stdin\n"
           , prog, INSTRUMENT);
    exit(1);
}



#if 0
static void
_checkerr(int d)
{
    char buf[16];
    int status;

    /* check error status */
    gpib_ibwrtf(d, "%s", SR620_ERROR_STATUS);
    gpib_ibrdstr(d, buf, sizeof(buf));
    if (*buf < '0' || *buf > '9') {
        fprintf(stderr, "%s: error reading error status byte\n", prog);
        exit(1);
    }
    status = strtoul(buf, NULL, 10);
    if (status & SR620_ERR_RECALL) 
        fprintf(stderr, "%s: recalled data was corrupt\n", prog);
    if (status & SR620_ERR_DELAY_RANGE)
        fprintf(stderr, "%s: delay range error\n", prog);
    if (status & SR620_ERR_DELAY_LINKAGE)
        fprintf(stderr, "%s: delay linkage error\n", prog);
    if (status & SR620_ERR_CMDMODE)
        fprintf(stderr, "%s: wrong mode for command\n", prog);
    if (status & SR620_ERR_ERANGE)
        fprintf(stderr, "%s: value out of range\n", prog);
    if (status & SR620_ERR_NUMPARAM)
        fprintf(stderr, "%s: wrong number of parameters\n", prog);
    if (status & SR620_ERR_BADCMD)
        fprintf(stderr, "%s: unrecognized command\n", prog);
    if (status != 0)
        exit(1);
}

static void
_checkinst(int d)
{
    char buf[16];
    int status;

    /* check instrument status */
    gpib_ibwrtf(d, "%s", SR620_INSTR_STATUS);
    gpib_ibrdstr(d, buf, sizeof(buf));
    if (*buf < '0' || *buf > '9') {
        fprintf(stderr, "%s: error reading instrument status byte\n", prog);
        exit(1);
    }
    status = strtoul(buf, NULL, 10);

    if (status & SR620_STAT_BADMEM)
        fprintf(stderr, "%s: memory contents corrupted\n", prog);
    if (status & SR620_STAT_SRQ)
        fprintf(stderr, "%s: service request\n", prog);
    if (status & SR620_STAT_TOOFAST)
        fprintf(stderr, "%s: trigger rate too high\n", prog);
    if (status & SR620_STAT_PLL)
        fprintf(stderr, "%s: 80MHz PLL is unlocked\n", prog);
    if (status & SR620_STAT_TRIG)
        fprintf(stderr, "%s: trigger has occurred\n", prog);
    if (status & SR620_STAT_BUSY)
        fprintf(stderr, "%s: busy with timing cycle\n", prog);
    if (status & SR620_STAT_CMDERR)
        fprintf(stderr, "%s: command error detected\n", prog);

    if (status != 0)
        exit(1);
}

static void
sr620_checkerr(int d)
{
    _checkerr(d);
    _checkinst(d);
}
#endif

static void
sr620_save(int d)
{
    char buf[MAXCONFBUF];
    int len;

    gpib_ibwrtf(d, "%s\r\n", SR620_SETUP_QUERY);
    len = gpib_ibrd(d, buf, sizeof(buf));
    if (write_all(1, buf, len) < 0) {
        perror("write");
        exit(1);
    }
}

/* Convert integer config string representation of "1,2,5 sequence"
 * back to float accepted by commands.
 */
static double
_decode125(int i)
{
    switch (i % 3) {
        case 0:
            return exp10(i / 3) * 1;
        case 1:
            return exp10(i / 3) * 2;
        case 2:
            return exp10(i / 3) * 5;
    }
    /*NOTREACHED*/
    return 0;
}

static void
sr620_restore(int d)
{
    char buf[MAXCONFBUF];
    int n;     
    int mode, srce, armm, gate;
    int size, disp, dgph, setup1;
    int setup2, setup3, setup4, hvscale;
    int hhscale, hbin, mgvert, jgvert;
    int setup5, setup6, rs232wait, ssetup;
    int ssd1, ssd0, shd2, shd1, shd0;

    if ((n = read_all(0, buf, MAXCONFBUF)) < 0) {
        perror("write");
        exit(1);
    }
    assert(n < MAXCONFBUF);
    buf[n] = '\0';
    n = sscanf(buf, 
            "%d,%d,%d,%d,"
            "%d,%d,%d,%d,"
            "%d,%d,%d,%d,"
            "%d,%d,%d,%d,"
            "%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d",
            &mode, &srce, &armm, &gate,
            &size, &disp, &dgph, &setup1,
            &setup2, &setup3, &setup4, &hvscale,
            &hhscale, &hbin, &mgvert, &jgvert,
            &setup5, &setup6, &rs232wait, &ssetup,
            &ssd1, &ssd0, &shd2, &shd1, &shd0);
    if (n != 25) {
        fprintf(stderr, "%s: error scanning configuration string\n", prog);
        exit(1);
    }
    gpib_ibwrtf(d, "%s %d", SR620_MODE, mode);
    gpib_ibwrtf(d, "%s %d", SR620_SRCE, srce);
    gpib_ibwrtf(d, "%s %d", SR620_TRIG_ARMMODE, armm);
    gpib_ibwrtf(d, "%s %.0E", SR620_GATE, _decode125(gate)); // XXX untested

    gpib_ibwrtf(d, "%s %.0E", SR620_SIZE, _decode125(size));
    // XXX unfinished
}

static void
sr620_checkid(int d)
{
    char tmpbuf[MAXIDBUF];

    gpib_ibwrtf(d, "%s", SR620_ID_QUERY);

    gpib_ibrdstr(d, tmpbuf, sizeof(tmpbuf));
    //hp3488_checksrq(d, NULL);

    if (strncmp(tmpbuf, SR620_ID_RESPONSE, strlen(SR620_ID_RESPONSE)) != 0) {
        fprintf(stderr, "%s: unsupported instrument: %s", prog, tmpbuf);
        exit(1);
    }
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int c, d;
    int clear = 0;
    int local = 0;
    int save = 0;
    int restore = 0;

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
        case 's': /* --save */
            save = 1;
            break;
        case 'r': /* --restore */
            restore = 1;
            break;
        default:
            usage();
            break;
        }
    }

    if (!clear && !local && !save && !restore)
        usage();

    /* find device in /etc/gpib.conf */
    d = gpib_init(prog, instrument, verbose);

    /* Set instrument to default configurations.
     * (same as holding down "clr rel" at power on)
     */
    if (clear) {
        gpib_ibclr(d);
        //sr620_checkerr(d);
        gpib_ibwrtf(d, "%s", SR620_RESET);
        sr620_checkid(d);
    }

    if (save) {
        sr620_save(d);
    }

    if (restore) {
        sr620_restore(d);
    }

    /* return front panel if requested */
    if (local)
        gpib_ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
