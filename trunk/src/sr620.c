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
#include <stdint.h>

#include "sr620.h"
#include "gpib.h"
#include "util.h"

#define INSTRUMENT "sr620"

#define MAXCONFBUF  256
#define MAXIDBUF    80

#define SR620_GENERAL_TIMEOUT 10    /* 10s general timeout */
#define SR620_AUTOCAL_TIMEOUT 180   /* 180s for autocal */

char *prog = "";
static int verbose = 0;

static strtab_t _selftest_errors[] = SR620_TEST_ERRORS;
static strtab_t _autocal_errors[] = SR620_AUTOCAL_ERRORS;

#define OPTIONS "a:clvsrStpPjCgG"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"save",            no_argument,       0, 's'},
    {"restore",         no_argument,       0, 'r'},
    {"selftest",        no_argument,       0, 'S'},
    {"autocal",         no_argument,       0, 't'},
    {"graph-histogram", no_argument,       0, 'p'},
    {"graph-mean",      no_argument,       0, 'P'},
    {"graph-jitter",    no_argument,       0, 'j'},
    {"graph-clear",     no_argument,       0, 'C'},
    {"graph-enable",    no_argument,       0, 'G'},
    {"graph-disable",   no_argument,       0, 'g'},
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
"  -a,--address addr     instrument address [%s]\n"
"  -c,--clear            initialize instrument to default values\n"
"  -l,--local            return instrument to local operation on exit\n"
"  -v,--verbose          show protocol on stderr\n"
"  -s,--save             save instrument setup to stdout\n"
"  -r,--restore          restore instrument setup from stdin\n"
"  -S,--selftest         run instrument self test\n"
"  -t,--autocal          run autocal procedure\n"
"  -C,--graph-clear      clear graph\n"
"  -G,--graph-enable     enable graph mode\n"
"  -g,--graph-disable    disable graph mode (improves measurement throughput)\n"
"  -p,--graph-histogram  graph histogram\n"
"  -P,--graph-mean       graph mean stripchart\n"
"  -j,--graph-jitter     graph jitter stripchart\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static int
_check_esr(gd_t gd, char *msg)
{
    int err = 0;
    int esb;

    gpib_wrtf(gd, "%s", SR620_ESR_QUERY);
    if (gpib_rdf(gd, "%d", &esb) != 1) {
        fprintf(stderr, "%s: error parsing ESR query response\n", prog);
        exit(1);
    }

    if ((esb & SR620_ESR_OPC)) {
        /* operation complete */    
    }
    if ((esb & SR620_ESR_QUERY_ERR)) {
        fprintf(stderr, "%s: %s: query error\n", prog, msg);
        err = 1;
    }
    if ((esb & SR620_ESR_EXEC_ERR)) {
        fprintf(stderr, "%s: %s: execution error\n", prog, msg);
        err = 1;
    }
    if ((esb & SR620_ESR_CMD_ERR)) {
        fprintf(stderr, "%s: %s: command error\n", prog, msg);
        err = 1;
    }
    if ((esb & SR620_ESR_CMD_URQ)) {
        /* keypress or trigger knob rotation */
    }
    if ((esb & SR620_ESR_CMD_PON)) {
        /* power on */
    }

    return err;
}

static int
_check_tic(gd_t gd, char *msg)
{
    int err = 0;

    return err;
}

static int
_check_error(gd_t gd, char *msg)
{
    int err = 0;
    int error;

    gpib_wrtf(gd, "%s", SR620_ERROR_QUERY);
    if (gpib_rdf(gd, "%d", &error) != 1) {
        fprintf(stderr, "%s: error parsing error query response\n", prog);
        exit(1);
    }

    if ((error & SR620_ERROR_PRINT)) {
        fprintf(stderr, "%s: print error\n", prog);
        err = 1;
    }
    if ((error & SR620_ERROR_NOCLOCK)) {
        fprintf(stderr, "%s: no clock\n", prog);
        err = 1;
    }
    if ((error & SR620_ERROR_AUTOLEV_A)) {
        fprintf(stderr, "%s: trigger lost on channel A\n", prog);
        err = 1;
    }
    if ((error & SR620_ERROR_AUTOLEV_B)) {
        fprintf(stderr, "%s: trigger lost on channel B\n", prog);
        err = 1;
    }
    if ((error & SR620_ERROR_TEST)) {
        fprintf(stderr, "%s: self test error\n", prog);
        err = 1;
    }
    if ((error & SR620_ERROR_CAL)) {
        fprintf(stderr, "%s: autocal error\n", prog);
        err = 1;
    }
    if ((error & SR620_ERROR_WARMUP)) {
        /* unit warmed up */
    }
    if ((error & SR620_ERROR_OVFLDIV0)) {
        fprintf(stderr, "%s: interval counter overflow/ratio mode divide by zero\n",
                prog);
        err = 1;
    }

    return err;
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if ((status & SR620_STATUS_ERROR)) {
        err = _check_error(gd, msg);
    }
    if ((status & SR620_STATUS_TIC)) {
        err = _check_tic(gd, msg);
    }
    if ((status & SR620_STATUS_MAV)) {
        /* data available in gpib buffer - ignore */
    }
    if ((status & SR620_STATUS_ESB)) {
        err = _check_esr(gd, msg);
        /* check ESB */
    }
    if ((status & SR620_STATUS_RQS)) {
    }
    if (err == 0 && !(status & SR620_STATUS_READY)) {
        //err = -1;
    } 
    if (err == 0 && !(status & SR620_STATUS_SREADY)) {
        //err = -1;
    }
    if (err == 0 && !(status & SR620_STATUS_PREADY)) {
        //err = -1;
    }
    return err;
}

/* Convert integer config string representation of "1,2,5 sequence"
 * back to float accepted by commands.
 */
static double
_decode125(int i)
{
    switch (i % 3) {
        case 0: return pow(10.0, i / 3) * 1;
        case 1: return pow(10.0, i / 3) * 2;
        case 2: return pow(10.0, i / 3) * 5;
    }
    /*NOTREACHED*/
    return 0;
}

static int
_decode_scanpt(int i)
{
    switch (i) {
        case 0: return 2;
        case 1: return 5;
        case 2: return 10;
        case 3: return 25;
        case 4: return 50;
        case 5: return 125;
        case 6: return 250;
    }
    return 0;
}

/* Restore instrument state from stdin.
 */
static void
sr620_restore(gd_t gd)
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
    /*1*/   "%d,%d,%d,%d,"
    /*5*/   "%d,%d,%d,%d,"
    /*9*/   "%d,%d,%d,%d,"
    /*13*/  "%d,%d,%d,%d,"
    /*17*/  "%d,%d,%d,%d,"
    /*21*/  "%d,%d,%d,%d,%d",
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
    /* 1 - instrument mode - same as MODE command */       
    gpib_wrtf(gd, "%s %d", SR620_MODE, mode);
    /* 2 - source - same as SRCE command */       
    gpib_wrtf(gd, "%s %d", SR620_SRCE, srce);
    /* 3 - arming mode - same as ARMM mode */       
    gpib_wrtf(gd, "%s %d", SR620_ARMM, armm);
    /* 4 - gate multiplier - same as front panel control (0=-1E-4, 1=2E-4...) */
    //gpib_wrtf(gd, "%s %.0E", SR620_GATE, _decode125(gate));
    /* 5 - sample size - 0,1,...,18 corresponding to 1,2,5,...,10^6 */
    gpib_wrtf(gd, "%s %.0E", SR620_SIZE, _decode125(size));
    /* 6 - display source - same as DISP command */
    gpib_wrtf(gd, "%s %d", SR620_DISP, disp);
    /* 7 - graph source - same as DGPH command */       
    gpib_wrtf(gd, "%s %d", SR620_DGPH, dgph);
    /** 
     ** 8 - setup byte 1 
     **/
    /* 8[0] - auto measure on/off */    
    gpib_wrtf(gd, "%s %d", SR620_AUTM, setup1 & 1);
    /* 8[1] - autoprint on/off */    
    gpib_wrtf(gd, "%s %d", SR620_AUTP, (setup1 >> 1) & 1);
    /* 8[2] - rel on/off */    
    gpib_wrtf(gd, "%s %d", SR620_DREL, (setup1 >> 2) & 1);
    /* 8[3] - x1000 on/off */    
    //gpib_wrtf(gd, "%s %d", SR620_EXPD, (setup1 >> 3) & 1);
    /* 8[4] - arming parity (+-time) */
    /* XXX COMP toggles parity but not sure how to set to particular value */
    /* 8[5] - jitter type (Allan/std dev) */
    gpib_wrtf(gd, "%s %d", SR620_JTTR, (setup1 >> 5) & 1);
    /* 8[6] - clock int/ext */
    gpib_wrtf(gd, "%s %d", SR620_CLCK, (setup1 >> 6) & 1);
    /* 8[7] - clock freq 10/5 mhz */
    gpib_wrtf(gd, "%s %d", SR620_CLKF, (setup1 >> 7) & 1);
    /**
     ** 9 - setup byte 2 
     **/
    /* 9[0] - A autolev on/off */
    gpib_wrtf(gd, "%s 1,%d", SR620_TMOD, setup2 & 1);
    /* 9[1] - B autolev on/off */
    gpib_wrtf(gd, "%s 2,%d", SR620_TMOD, (setup2 >> 1) & 1);
    /* 9[2:3] - dvm0 gain (auto=0, 20V=1, 2V=2) */
    gpib_wrtf(gd, "%s 0,%d", SR620_RNGE, (setup2 >> 2) & 3);
    /* 9[4] - A prescaler enabled */
    /* XXX redundant with TERM below? */
    /* 9[5] - B prescaler enabled */
    /* XXX redundant with TERM below? */
    /* 9[6:7] - dvm1 gain (auto=0, 20V=1, 2V=2) */
    gpib_wrtf(gd, "%s 1,%d", SR620_RNGE, (setup2 >> 6) & 3);
    /**
     ** 10 - setup byte 3
     **/
    /* 10[0] - ext terminator */
    gpib_wrtf(gd, "%s 0,%d", SR620_TERM, setup3 & 1);
    /* 10[1] - ext slope */
    gpib_wrtf(gd, "%s 0,%d", SR620_TSLP, (setup3 >> 1) & 1);
    /* 10[2] - A slope */
    gpib_wrtf(gd, "%s 1,%d", SR620_TSLP, (setup3 >> 2) & 1);
    /* 10[3] - A ac/dc */
    gpib_wrtf(gd, "%s 1,%d", SR620_TCPL, (setup3 >> 3) & 1);
    /* 10[4] - B slope */
    gpib_wrtf(gd, "%s 2,%d", SR620_TSLP, (setup3 >> 4) & 1);
    /* 10[5] - B ac/dc */
    gpib_wrtf(gd, "%s 2,%d", SR620_TCPL, (setup3 >> 5) & 1);
    /**
     ** 11 - setup byte 4
     **/
    /* 11[0:1] - A terminator */
    gpib_wrtf(gd, "%s 1,%d", SR620_TERM, setup4 & 0x03);
    /* 11[2:3] - B terminator */
    gpib_wrtf(gd, "%s 2,%d", SR620_TERM, (setup4 >> 2) & 0x03);
    /* 11[4:5] - print port mode */
    gpib_wrtf(gd, "%s %d", SR620_PRTM, (setup4 >> 4) & 0x03);
    /* 12 - histogram vert scale - 0,1,... corresponding to 1,2,5,... */
    //gpib_wrtf(gd, "%s 0,%.0E", SR620_GSCL, _decode125(hvscale));
    /* 13 - histogram horiz scale - 1,2,5 led display LSD (ps, uHz, etc) */
    //gpib_wrtf(gd, "%s 1,%.0E", SR620_GSCL, _decode125(hhscale));
    /* 14 - histogram bin number */
    //gpib_wrtf(gd, "%s 2,%.0E", SR620_GSCL, _decode125(hbin));
    /* 15 - mean graph vert scale */
    //gpib_wrtf(gd, "%s 3,%.0E", SR620_GSCL, _decode125(mgvert));
    /* 16 - jitter graph vert scale */
    //gpib_wrtf(gd, "%s 4,%.0E", SR620_GSCL, _decode125(jgvert));
    /**
     ** 17 - setup byte 5
     **/
    /* 17[0:4] - plotter addr */
    gpib_wrtf(gd, "%s %d", SR620_PLAD, setup5 & 0x1f);
    /* 17[5] - plot/print */
    gpib_wrtf(gd, "%s %d", SR620_PDEV, (setup5 >> 5) & 1);
    /* 17[6] - plot gpib/rs232 */
    gpib_wrtf(gd, "%s %d", SR620_PLPT, (setup5 >> 6) & 1);
    /**
     ** 18 - setup byte 6
     **/
    /* 18[0-1] - d/a output mode (chart/d/a) */
    gpib_wrtf(gd, "%s %d", SR620_ANMD, setup6 & 3);
    /* 18[2] - graph on/off */
    gpib_wrtf(gd, "%s %d", SR620_GENA, (setup6 >> 2) & 1);
    /* 18[3:6] - #scan points */
    gpib_wrtf(gd, "%s %d", SR620_SCPT, _decode_scanpt((setup6 >> 3) & 0xf));
    /* 18[7] - ref output level */
    gpib_wrtf(gd, "%s %d", SR620_RLVL, (setup6 >> 7) & 1);
    /* 19 - rs232 wait - same as WAIT command */
    gpib_wrtf(gd, "%s %d", SR620_WAIT, rs232wait);
    /**
     ** 20 - scan setup byte
     **/
    /* 20[0:3] - stepsize 1E-6, 2E-6,... */
    //gpib_wrtf(gd, "%s %.0E", SR620_DSTP, _decode125(ssetup & 0xf)); // XXX?
    /* 20[4:5]*/ /* delay scan enable */
    gpib_wrtf(gd, "%s %d", SR620_SCEN, (ssetup >> 4) & 3); // XXX?
    /* 21,22 - scan starting delay = (256*byte21 + byte22) */
    gpib_wrtf(gd, "%s %d", SR620_DBEG, 256*ssd1 + ssd0);
    /* 23,24,25 - scan hold time = (65536*byte23 + 256*byte24 + byte25) */
    gpib_wrtf(gd, "%s %.0E", SR620_HOLD, 0.01*(65536*shd2 + 256*shd1 + shd0));
}

/* Save instrument state to stdout.
 */
static void
sr620_save(gd_t gd)
{
    char buf[MAXCONFBUF];

    gpib_wrtf(gd, "%s", SR620_SETUP_QUERY);
    gpib_rdstr(gd, buf, sizeof(buf));
    if (write_all(1, buf, strlen(buf)) < 0) {
        perror("write");
        exit(1);
    }
}


/* Run instrument self-test and report results on stderr.
 */
static void
sr620_test(gd_t gd)
{
    int result;

    gpib_wrtf(gd, "%s", SR620_SELFTEST);
    if (gpib_rdf(gd, "%d", &result) != 1) {
        fprintf(stderr, "%s: error parsing self test result\n", prog);
        exit(1);
    }
    if (result == 0) {
        fprintf(stderr, "%s: self test passed\n", prog);
        return;
    }
    fprintf(stderr, "%s: self test failed: %s\n", prog, 
            findstr(_selftest_errors, result));
    exit(1);
}

/* Run instrument autocal procedure.
 */
static void
sr620_autocal(gd_t gd)
{
    int result;

    gpib_wrtf(gd, "%s", SR620_AUTOCAL);
    fprintf(stderr, "%s: running autocal (takes ~3m)...\n", prog);
    gpib_set_timeout(gd, SR620_AUTOCAL_TIMEOUT);
    if (gpib_rdf(gd, "%d", &result) != 1) {
        fprintf(stderr, "%s: error parsing autocal result\n", prog);
        exit(1);
    }
    gpib_set_timeout(gd, SR620_GENERAL_TIMEOUT);
    if (result == 0) {
        fprintf(stderr, "%s: autocal complete\n", prog);
    } else {
        fprintf(stderr, "%s: autocal: %s\n", prog, 
            findstr(_autocal_errors, result));
        exit(1);
    }
}

/* Set instrument to default configurations and check identity.
 */
static void
sr620_clear(gd_t gd)
{
    char tmpbuf[80];


    gpib_clr(gd, 200000);
    gpib_wrtf(gd, "%s", SR620_RESET);

    /* check id */
    gpib_wrtf(gd, "%s", SR620_ID_QUERY);
    gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));

    if (strncmp(tmpbuf, SR620_ID_RESPONSE, strlen(SR620_ID_RESPONSE)) != 0) {
        fprintf(stderr, "%s: unsupported instrument: %s", prog, tmpbuf);
        exit(1);
    }

    /* set up error masks */
    gpib_wrtf(gd, "%s %d", SR620_ESR_ENABLE,
            (SR620_ESR_QUERY_ERR | SR620_ESR_EXEC_ERR | SR620_ESR_CMD_ERR));
    gpib_wrtf(gd, "%s %d", SR620_ERROR_ENABLE,
            (SR620_ERROR_PRINT | SR620_ERROR_NOCLOCK | 
             /*SR620_ERROR_AUTOLEV_A | SR620_ERROR_AUTOLEV_B |*/
             SR620_ERROR_TEST | SR620_ERROR_CAL | SR620_ERROR_OVFLDIV0));

    /* clear status on next power on */
    gpib_wrtf(gd, "%s 1", SR620_PSC); 
}

static void
sr620_graph_histogram(gd_t gd)
{
    gpib_wrtf(gd, "%s 0", SR620_DGPH);    /* select histogram display */
}

static void
sr620_graph_mean(gd_t gd)
{
    gpib_wrtf(gd, "%s 1", SR620_DGPH);    /* select mean schart display */
}

static void
sr620_graph_jitter(gd_t gd)
{
    gpib_wrtf(gd, "%s 2", SR620_DGPH);    /* select jitter schart display */
}

static void
sr620_graph_clear(gd_t gd)
{
    gpib_wrtf(gd, "%s", SR620_GCLR);      /* clear graphs */
}

static void
sr620_graph_enable(gd_t gd)
{
    gpib_wrtf(gd, "%s 1", SR620_GENA);    /* enable graphs */
}

static void
sr620_graph_disable(gd_t gd)
{
    gpib_wrtf(gd, "%s 0", SR620_GENA);    /* disable graphs */
}

int
main(int argc, char *argv[])
{
    char *addr = NULL;
    int c;
    int clear = 0;
    int local = 0;
    int save = 0;
    int restore = 0;
    int test = 0;
    int autocal = 0;
    int graph_histogram = 0;
    int graph_mean = 0;
    int graph_jitter = 0;
    int graph_clear = 0;
    int graph_enable = 0;
    int graph_disable = 0;
    gd_t gd;

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
        case 'S': /* --selftest */
            test = 1;
            break;
        case 't': /* --autocal */
            autocal = 1;
            break;
        case 'p': /* --graph-histogram */
            graph_histogram = 1;
            break;
        case 'P': /* --graph-mean */
            graph_mean = 1;
            break;
        case 'j': /* --graph-jitter */
            graph_jitter = 1;
            break;
        case 'C': /* --graph-clear */
            graph_clear = 1;
            break;
        case 'G': /* --graph-enable */
            graph_enable = 1;
            break;
        case 'g': /* --graph-disable */
            graph_disable = 1;
            break;
        default:
            usage();
            break;
        }
    }
    if (optind < argc)
        usage();

    if (!clear && !local && !save && !restore && !test && !autocal &&
            !graph_clear && !graph_enable && !graph_disable && 
            !graph_histogram && !graph_mean && !graph_jitter)
        usage();

    if (!addr)
        addr = gpib_default_addr(INSTRUMENT);
    if (!addr) {
        fprintf(stderr, "%s: no default address for %s, use --address\n",
                prog, INSTRUMENT);
        exit(1);
    }
    gd = gpib_init(addr, _interpret_status, 0);
    if (!gd) {
        fprintf(stderr, "%s: device initialization failed for address %s\n", 
                prog, addr);
        exit(1);
    }
    gpib_set_verbose(gd, verbose);
    gpib_set_timeout(gd, SR620_GENERAL_TIMEOUT);

    if (clear)
        sr620_clear(gd);

    if (test)
        sr620_test(gd);
    if (autocal)
        sr620_autocal(gd);
    if (save)
        sr620_save(gd);
    if (restore)
        sr620_restore(gd);
    if (graph_enable)
        sr620_graph_enable(gd);
    if (graph_clear)
        sr620_graph_clear(gd);
    if (graph_histogram)
        sr620_graph_histogram(gd);
    if (graph_mean)
        sr620_graph_mean(gd);
    if (graph_jitter)
        sr620_graph_jitter(gd);
    if (graph_disable)
        sr620_graph_disable(gd);

    if (local)
        gpib_loc(gd); 

    gpib_fini(gd);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
