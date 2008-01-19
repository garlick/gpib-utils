/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2007 Jim Garlick <garlick.jim@gmail.com>

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

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdarg.h>
#include <libgen.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <assert.h>

#include "gpib.h"
#include "util.h"
#include "argv.h"

char *prog;

static gd_t gd = NULL;
static unsigned char ignoremask = 0;   /* status bits which are not errors */
static unsigned char readymask = 0;    /* status bits which indicate ready */
static unsigned long ibclrusec = 2000000; /* usecs to pause after ibclr */

#define OPTIONS ""
static struct option longopts[] = {
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    fprintf(stderr, "Usage: %s\n", prog);
    exit(1);
}

static void 
_help(void)
{
    printf("=====================================================================\n");
    printf("open name|addr   Open device using name or primary address.\n");
    printf("close            Close device.\n");
    printf("get              List parameter values.\n");
    printf("set [parm=val]   Set parameter to value/list parameter help.\n");
    printf("clr              Send GPIB device-clear message.\n");
    printf("loc              Send GPIB return-to-local message.\n");
    printf("write <string>   Send string to device.\n");
    printf("read             Read string from device\n");
    printf("quit             Exit.\n");
    printf("=====================================================================\n");
}

static void
_help_params(void)
{
    printf("=====================================================================\n");
    printf("timeout     GPIB timeout in seconds [0 - 1000]\n");
    printf("eot         assert EOI with last byte on writes [0, 1]\n");
    printf("reos        terminate reads on reception of end-of-string char [0, 1]\n");
    printf("xeos        assert EOI whenever end-of-string char is sent [0, 1]\n");
    printf("bin         use 8 bits to match end-of-string char [0, 1]\n");
    printf("eos         end-of-string character [0 - 0xff]\n");
    printf("verbose     show GPIB conversation on stderr [0, 1]\n");
    printf("ignoremask  bits to ignore in status byte [0 - 0xff]\n");
    printf("readymask   bits that indicate ready in status byte [0 - 0xff]\n");
    printf("ibclrusec   number of usec to pause between ibclr and serial poll [0 - big!]\n");
    printf("=====================================================================\n");
}

/* Interpet serial poll results (status byte)
 *   Return: 0=non-fatal/no error, >0=fatal, -1=retry.
 */
static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;
    unsigned char s = status & ~(ignoremask | readymask);

    if (s != 0) {
        fprintf(stderr, "%s: status byte error (0x%x)\n", prog, s);
        err = 0; /* make it non-fatal for interactive use */
    } else if (readymask && !(status & readymask)) {
        err = -1;
    }

    return err;
}

static void
_open(char *addr)
{
    if (!gd) {
        if (!(gd = gpib_init(addr, _interpret_status, 100000)))
           printf("couldn't initialize device: %s\n", addr);
    } else {
        printf("please close the device first\n");
        _help();
    }
}

static void
_close(void)
{
    if (gd) {
        gpib_fini(gd);
        gd = NULL;
    } else {
        printf("please open a device first\n");
    }
}

static void
_recv(void)
{
    char buf[1024];
    char *nstr;

    if (gd) {
        gpib_rdstr(gd, buf, sizeof(buf));
        if (strlen(buf) > 0) {
            nstr = xstrcpyprint(buf);
            printf("%s\n", nstr);
            free(nstr);
        }
    } else {
        printf("please open a device first\n");
    }
} 

static void
_send(char *str)
{
    char *nstr; 
    
    if (gd) {
        nstr = xstrcpyunprint(str);
        gpib_wrtstr(gd, nstr);
        free(nstr);
    } else {
        printf("please open a device first\n");
    }
}

static void
_get_params(void)
{
    if (gd) {
        printf("timeout     %lfs\n", gpib_get_timeout(gd));
        printf("eot         %d\n", gpib_get_eot(gd));
        printf("reos        %d\n", gpib_get_reos(gd));
        printf("xeos        %d\n", gpib_get_xeos(gd));
        printf("bin         %d\n", gpib_get_bin(gd));
        printf("eos         0x%x\n", gpib_get_eos(gd));
        printf("verbose     %d\n", gpib_get_verbose(gd));
        printf("ignoremask  0x%x\n", ignoremask);
        printf("readymask   0x%x\n", readymask);
        printf("ibclrusec   %lu\n", ibclrusec);
    } else {
        printf("please open a device first\n");
    }
}

static void
_set_param(char *key, char *val)
{
    if (gd) {
        if (key) {
            if (!strcmp(key, "verbose")) {
                gpib_set_verbose(gd, val ? strtoul(val, NULL, 10) : 1);
            } else if (!strcmp(key, "timeout")) {
                if (val)
                    gpib_set_timeout(gd, strtod(val, NULL));
                else
                    printf("timeout: requires an argument (secs)\n");
            } else if (!strcmp(key, "eot")) {
                gpib_set_eot(gd, val ? strtoul(val, NULL, 10) : 1);
            } else if (!strcmp(key, "reos")) {
                gpib_set_reos(gd, val ? strtoul(val, NULL, 10) : 1);
            } else if (!strcmp(key, "xeos")) {
                gpib_set_xeos(gd, val ? strtoul(val, NULL, 10) : 1);
            } else if (!strcmp(key, "bin")) {
                gpib_set_bin(gd, val ? strtoul(val, NULL, 10) : 1);
            } else if (!strcmp(key, "eos")) {
                if (val)
                    gpib_set_eos(gd, strtoul(val, NULL, 0));
                else
                    printf("eos: requires an argument\n");
            } else if (!strcmp(key, "ignoremask")) {
                ignoremask = val ? strtoul(val, NULL, 0) : 0;
            } else if (!strcmp(key, "readymask")) {
                readymask = val ? strtoul(val, NULL, 0) : 0;
            } else if (!strcmp(key, "ibclrusec")) {
                ibclrusec = val ? strtoul(val, NULL, 10) : 0;
            } else 
                printf("unknown set key: %s\n", key);
        } else {
            _help_params();
        }
    } else {
        printf("please open a device first\n");
    }
}


static int
docmd(char **av)
{
    int quit = 0;

    assert(av[0] != NULL);
    if (!strcmp(av[0], "?") || !strcmp(av[0], "help")) {
        _help(); 
    } else if (!strcmp(av[0], "open")) {
        if (av[1] == NULL || av[2] != NULL)
            printf("Usage: open device\n");
        else 
            _open(av[1]);
    } else if (!strcmp(av[0], "close")) {
        if (av[1] != NULL) 
            printf("Usage: close\n");
        else 
            _close();
    } else if (!strcmp(av[0], "clear")) {
        if (av[1] != NULL)
            printf("Usage: clear\n");
        else if (!gd)
            printf("Please open a device first\n");
        else
            gpib_clr(gd, ibclrusec);
    } else if (!strcmp(av[0], "local")) {
        if (av[1] != NULL)
            printf("Usage: local\n");
        else if (!gd)
            printf("Please open a device first\n");
        else
            gpib_loc(gd);
    } else if (!strcmp(av[0], "quit")) {
        quit = 1;
    } else {
        printf("Unknown command: type \"?\" for a list\n");
    }
    return quit;
}

static void
_shell(void)
{
    char *line;
    char **av;
    int quit = 0;

    /* disable readline file name completion */
    /*rl_bind_key ('\t', rl_insert); */

    while (!quit && (line = readline("gpsh> "))) {
        if (strlen(line) > 0) {
            add_history(line); 
            av = argv_create(line, "");
            if (av[0] != NULL)
                quit = docmd(av);
            argv_destroy(av);
        } 
        free(line);
    }
}

int
main(int argc, char *argv[])
{
    int c;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        default:
            usage();
            break;
        }
    }

    if (optind < argc)
        usage();

    _shell();

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
