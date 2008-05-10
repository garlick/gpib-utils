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

#include "gpib.h"
#include "util.h"

#define INSTRUMENT "hp6632"

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

char *prog = "";
static int verbose = 0;

#define OPTIONS "a:clviSI:V:o:O:C:r"
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"ident",           no_argument,       0, 'i'},
    {"selftest",        no_argument,       0, 'S'},
    {"iset",            required_argument, 0, 'I'},
    {"vset",            required_argument, 0, 'V'},
    {"out",             required_argument, 0, 'o'},
    {"ovset",           required_argument, 0, 'O'},
    {"ocp",             required_argument, 0, 'C'},
    {"read",            no_argument,       0, 'r'},
    {0, 0, 0, 0},
};

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
"  -i,--ident              print instrument model and ROM version\n"
"  -S,--selftest           run selftest\n"
"  -I,--iset               set current\n"
"  -V,--vset               set voltage\n"
"  -o,--out                enable/disable output (0|1)\n"
"  -O,--ovset              set overvoltage threshold\n"
"  -C,--ocp                enable/disable overcurrent protection (0|1)\n"
"  -r,--read               read actual current and voltage\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;
    int e;

    if (status & HP6032_SPOLL_FAU) {
        fprintf(stderr, "%s: device fault\n", prog);
        /* FIXME: check FAULT? and/or ASTS? but only if unmasked */
        err = 1;
    }
    if (status & HP6032_SPOLL_PON) {
        fprintf(stderr, "%s: power-on detected\n", prog);
    }
    if (status & HP6032_SPOLL_RDY) {
    }
    if (status & HP6032_SPOLL_ERR) {
        gpib_wrtstr(gd, "ERR?\n");
        if (gpib_rdf(gd, "%d", &e) != 1)
            fprintf(stderr, "%s: prog error and error querying ERR?\n", prog);
        else
            fprintf(stderr, "%s: prog error: %s\n", prog, findstr(petab, e));
        err = 1;
    }
    if (status & HP6032_SPOLL_RQS) {
        fprintf(stderr, "%s: device is requesting service\n", prog);
        err = 1; /* it shouldn't be unless we tell it to */
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

    return rc;
}

/* Read current and voltage at output and print in a form
 * suitable for gnuplot usage: "current<tab>voltage"
 */
static int 
_read(gd_t gd)
{
    float i, v;

    gpib_wrtstr(gd, "VOUT?\n");
    if (gpib_rdf(gd, "%f", &v) != 1) {
        fprintf(stderr, "%s: error reading VOUT? result\n", prog);
        return -1;
    }
    gpib_wrtstr(gd, "IOUT?\n");
    if (gpib_rdf(gd, "%f", &i) != 1) {
        fprintf(stderr, "%s: error reading IOUT? result\n", prog);
        return -1;
    }
    printf("%f\t%f\n", i, v);
    return 0;
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

static int
_str2float(char *str, float *fp)
{
    char *endptr;

    errno = 0;
    *fp = strtof(str, &endptr);
    if (errno || *endptr != '\0') {
        fprintf(stderr, "%s: error parsing float value\n", prog);
        return 1;
    }
    return 0;
}

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
    int iset = 0;
    int vset = 0;
    int ovset = 0;
    float ovset_val, iset_val, vset_val;
    int ocp = 0;
    int ocp_val;
    int ropt = 0;

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
        case 'I': /* --iset */
            iset = 1;
            if (_str2float(optarg, &iset_val))
                exit(1);
            todo++;
            break;
        case 'V': /* --vset */
            vset = 1;
            if (_str2float(optarg, &vset_val))
                exit(1);
            todo++;
            break;
        case 'O': /* --ovset */
            ovset = 1;
            if (_str2float(optarg, &ovset_val))
                exit(1);
            todo++;
            break;
        case 'o': /* --ocp */
            ocp = 1;
            if (*optarg == '0')
                ocp_val = 0;
            else if (*optarg == '1')
                ocp_val = 1;
            else {
                fprintf(stderr, "%s: error parsing --ocp argument\n", prog);
                exit(1);
            }
            todo++;
            break;
        case 'r': /* --read */
            ropt = 1;
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
    if (ovset)
        gpib_wrtf(gd, "OVSET %f\n", ovset_val);
    if (iset)
        gpib_wrtf(gd, "ISET %f\n", iset_val);
    if (vset)
        gpib_wrtf(gd, "VSET %f\n", vset_val);
    if (ocp)
        gpib_wrtf(gd, "OCP %d\n", ocp_val);
    if (ropt && _read(gd) != 0) {
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
