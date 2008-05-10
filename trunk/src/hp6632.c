/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2008 Jim Garlick <garlick@speakeasy.net>

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

/* HP 6632A (25V, 4A); 6633A (50V, 2A); 6634A (100V, 1A) power supplies */

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

#include "units.h"
#include "gpib.h"
#include "util.h"

#define INSTRUMENT "hp6632"

char *prog = "";
static int verbose = 0;

#define OPTIONS "a:clviS"
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"get-idn",         no_argument,       0, 'i'},
    {"selftest",        no_argument,       0, 'S'},
    {0, 0, 0, 0},
};

/* programming errors */
static strtab_t petab[] = {
    { 0, "Success" },
    { 1, "EEPROM save failed" },
    { 2, "Second PON after power-on" },
    { 4, "Second DC PON after power-on" },
    { 5, "No relay option present" },
    { 8, "Addressed to talk and nothing to say" },
    { 10, "Header expected" },
    { 11, "Unrecognized header" },
    { 20, "Number expected" },
    { 21, "Number syntax" },
    { 22, "Number out of internal range" },
    { 30, "Comma expected" },
    { 31, "Terminator expected" },
    { 41, "Parameter out of range" },
    { 42, "Voltage programming error" },
    { 43, "Current programming error" },
    { 44, "Overvoltage programming error" },
    { 45, "Delay programming error" },
    { 46, "Mask programming error" },
    { 50, "Multiple CSAVE commands" },
    { 51, "EEPROM checksum failure" },
    { 52, "Calibration mode disabled" },
    { 53, "CAL channel out of range" },
    { 54, "CAL FS out of range" },
    { 54, "CAL offset out of range" },
    { 54, "CAL disable jumper in" },
    { 0, NULL }
};

/* selftest failures */
static strtab_t sttab[] = {
    { 0, "Self-test passed" },
    { 1, "ROM checksum failure" },
    { 2, "RAM test failure" },
    { 3, "HP-IB chip failure" },
    { 4, "HP-IB microprocessor timer slow" },
    { 5, "HP-IB microprocessor timer fast" },
    { 11, "PSI ROM checksum failure" },
    { 12, "PSI RAM test failure" },
    { 14, "PSI microprocessor timer slow" },
    { 15, "PSI microprocessor timer fast" },
    { 16, "A/D test reads high" },
    { 17, "A/D test reads low" },
    { 18, "CV/CC zero too high" },
    { 19, "CV/CC zero too low" },
    { 20, "CV ref FS too high" },
    { 21, "CV ref FS too low" },
    { 22, "CC ref FS too high" },
    { 23, "CC ref FS too low" },
    { 24, "DAC test failed" },
    { 51, "EEPROM checksum failed" },
    { 0, NULL }
};

/* status, astatus, fault, mask reg bit definitions */
#define HP6032_STAT_CV      1       /* constant voltage mode */
#define HP6032_STAT_CC      2       /* positive constant current mode */
#define HP6032_STAT_UNR     4       /* output unregulated */
#define HP6032_STAT_OV      8       /* overvoltage protection tripped */
#define HP6032_STAT_OT      16      /* over-temperature protection tripped */
#define HP6032_STAT_OC      64      /* overcurrent protection tripped */
#define HP6032_STAT_ERR     128     /* programming error */
#define HP6032_STAT_INH     256     /* remote inhibit */
#define HP6032_STAT_NCC     512     /* negative constant current mode */
#define HP6032_STAT_FAST    1024    /* output in fast operating mode */
#define HP6032_STAT_NORM    2048    /* output in normal operating mode */

/* serial poll reg */
#define HP6032_SPOLL_FAU    1
#define HP6032_SPOLL_PON    2
#define HP6032_SPOLL_RDY    16
#define HP6032_SPOLL_ERR    32
#define HP6032_SPOLL_RQS    64

static void 
usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -a,--address            set instrument address [%s]\n"
"  -c,--clear              initialize instrument to default values\n"
"  -l,--local              return instrument to local operation on exit\n"
"  -v,--verbose            show protocol on stderr\n"
"  -i,--get-idn            return instrument idn string\n"
"  -S,--selftest           run selftest\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if (status & HP6032_SPOLL_FAU) {
        fprintf(stderr, "%s: device fault\n", prog);
        /* FIXME: check FAULT? */
        /* FIXME: check ASTS? */
        err = 1;
    }
    if (status & HP6032_SPOLL_PON) {
        fprintf(stderr, "%s: power-on detected\n", prog);
    }
    if (status & HP6032_SPOLL_RDY) {
    }
    if (status & HP6032_SPOLL_ERR) {
        fprintf(stderr, "%s: programming error\n", prog);
        /* FIXME: check ERR? */
    }
    if (status & HP6032_SPOLL_RQS) {
        fprintf(stderr, "%s: device is requesting service\n", prog);
    }
    return err;
}

/* Print instrument ID string.
 */
static int
_get_idn(gd_t gd)
{
    char model[64], rom[64];
    int rc = 0;

    gpib_wrtstr(gd, "ID?\n");
    gpib_rdstr(gd, model, sizeof(model));
    gpib_wrtstr(gd, "ROM?\n");
    gpib_rdstr(gd, rom, sizeof(rom));
    fprintf(stderr, "%s: %s, %s\n", prog, model, rom);
    if (strcmp(model, "6032") != 0 && strcmp(model, "6033") != 0
                                   && strcmp(model, "6034") != 0) {
        fprintf(stderr, "%s: not a supported model\n", prog);
        rc = 1;
    }
    return rc;
}

/* Run instrument self-test.
 */
static int
_selftest(gd_t gd)
{
    int rc;

    gpib_wrtstr(gd, "TEST?\n");
    if (gpib_rdf(gd, "%d", &rc) != 1) {
        fprintf(stderr, "%s: parse error reading test result\n", prog);
        return -1;
    }
    fprintf(stderr, "%s: selftest: %s\n", prog, findstr(sttab ,rc));
            
    return rc;
}

/* Cheat sheet:
 * OUT 0;RST;CLR
 * VSET %f;ISET %f;OVSET %f;OCP %d
 * VSET %f
 * IOUT?
 * VOUT?
 */

int
main(int argc, char *argv[])
{
    gd_t gd;
    char *addr = NULL;
    int c;
    int clear = 0;
    int local = 0;
    int todo = 0;
    int exit_val = 0;
    int get_idn = 0;
    int selftest = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'a': /* --address */
            addr = optarg;
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        case 'c': /* --clear */
            clear = 1;
            todo++;
            break;
        case 'l': /* --local */
            local = 1;
            todo++;
            break;
        case 'i': /* --get-idn */
            get_idn = 1;
            todo++;
            break;
        case 'S': /* --selftest */
            selftest = 1;
            todo++;
            break;
        default:
            usage();
            break;
        }
    }
    if (optind < argc || !todo)
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

    if (clear) {
        gpib_clr(gd, 0);
        gpib_wrtf(gd, "OUT 0;RST;CLR\n");
        sleep(1);
    }
    if (get_idn && _get_idn(gd) != 0) {
        exit_val = 1;
        goto done;
    }
    if (selftest && _selftest(gd) != 0) {
        exit_val = 1;
        goto done;
    }
    if (local)
        gpib_loc(gd); 

done:
    gpib_fini(gd);
    exit(exit_val);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */