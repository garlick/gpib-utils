/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2009 Jim Garlick <garlick.jim@gmail.com>

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

/* ibquery - generic instrument query utility */

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

#include "util.h"
#include "gpib.h"

#define MAX_RESPONSE_LEN	4096

char *prog = "";
const char *options = OPTIONS_COMMON "lcw:rq:s:d:";

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    OPTIONS_COMMON_LONG,
    {"local",           no_argument,       0, 'l'},
    {"clear",           no_argument,       0, 'c'},
    {"write",           required_argument, 0, 'w'},
    {"read",            no_argument,       0, 'r'},
    {"query",           required_argument, 0, 'q'},
    {"status",          required_argument, 0, 's'},
    {"delay",           required_argument, 0, 'd'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    {"l", "local",   "return instrument to front panel control" },
    {"c", "clear",   "send device clear message" },
    {"w", "write",   "write to instrument" },
    {"r", "read",    "read from instrument" },
    {"q", "query",   "write to instrument then read response" },
    {"s", "status",  "apply mask to status byte, exit if non-zero" },
    {"d", "delay",   "delay the specified number of seconds" },
    {0, 0},
};

int
main(int argc, char *argv[])
{
    int exit_val = 0;
    int print_usage = 0;
    gd_t gd;
    int c;
    char buf[MAX_RESPONSE_LEN];
    unsigned char status, mask;

    gd = gpib_init_args(argc, argv, options, longopts, NULL,
                        NULL, 0, &print_usage);
    if (print_usage)
        usage(optdesc);
    if (!gd)
        exit(1);

    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH
                break;
            case 'l': /* --local */
                gpib_loc(gd);
                break;
            case 'c': /* --clear */
                gpib_clr(gd, 100000); /* built-in delay of 100ms */
                break;
            case 'w': /* --write */
                gpib_wrtstr(gd, optarg);
                break;
            case 'r': /* --read */
                gpib_rdstr(gd, buf, sizeof(buf));
                printf("%s\n", buf);
                break;
            case 'q': /* --query */
                (void)gpib_qrystr(gd, optarg, buf, sizeof(buf));
                printf("%s\n", buf);
                break;
            case 's': /* --status */
                mask = (unsigned char)strtoul(optarg, NULL, 0);
                (void)gpib_rsp(gd, &status);
                printf("%hhu\n", status);
                if ((status & mask) != 0)
                    exit(1);
                break;
            case 'd': /* --delay */
                usleep((useconds_t)(strtod(optarg, NULL) * 1000000.0));
                break;
        }
    }

    gpib_fini(gd);
    exit(exit_val);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
