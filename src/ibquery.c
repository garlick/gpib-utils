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

#include "libutil/util.h"
#include "libutil/argv.h"
#include "libinst/inst.h"
#include "libinst/configfile.h"

#define MAX_RESPONSE_LEN	4096

char *prog = "";
const char *options = "vlcw:rq:s:d:";

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"verbose",         no_argument,       0, 'v'},
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

void usage (void)
{
    fprintf (stderr, "%s", "Usage: ibquery NAME [OPTIONS]\n"
        "    -v,--verbose set verbose flag\n");
    exit (1);
};

int command (struct instrument *gd, int ac, char **av);

int
main(int argc, char *argv[])
{
    struct instrument *gd = NULL;
    int c;
    char *name = NULL;
    struct cf_file *cf = NULL;
    const struct cf_instrument *cfi = NULL;
    int verbose = 0;

    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            case 'v':
                verbose++;
                break;
        }
    }
    if (optind < argc)
        name = argv[optind++];

    cf = cf_create_default ();
    if (!cf) {
        fprintf (stderr, "Unable to parse config file\n");
        exit (1);
    }
    if (!name) {
        cf_list (cf, stdout);
    } else {
        if (!(cfi = cf_lookup (cf, name))) {
            fprintf (stderr, "No instrument named %s is configured\n", name);
            exit (1);
        }
        if (verbose)
            fprintf (stderr, "%s: using address %s\n", cfi->name, cfi->addr);
        if (!(gd = inst_init (cfi->addr, NULL, 0))) {
            fprintf (stderr, "Could not initialize %s at %s\n", cfi->name,
                                                                cfi->addr);
            exit (1);
        }
        if (verbose) {
            fprintf (stderr, "%s: initialized\n", cfi->name);
            inst_set_verbose (gd, 1);
        }
        if (cfi->flags & GPIB_FLAG_REOS) {
            if (verbose)
                fprintf (stderr, "%s: setting reos flag\n", cfi->name);
            inst_set_reos (gd, 1);
        }
        if (optind < argc) {
            while (optind < argc) {
                optind += command (gd, argc - optind, argv + optind);
            }
        } else for (;;) {
            static char buf[128];
            char **av;
            xreadline ("ibquery> ", buf, sizeof (buf));
            av = argv_create (buf, "");
            if (!av[0])
                continue;
            if (!strcmp (av[0], "quit") || !strcmp (av[0], "exit"))
                break;
            int ac = 0;
            char **ap;
            for (ap = &av[0]; *ap != NULL; ap++)
                ac++;
            command (gd, ac, av);
            argv_destroy (av);
        }
        inst_fini(gd);
    }

    cf_destroy (cf);
    exit (0);
}

int command (struct instrument *gd, int ac, char **av)
{
    int count = 0;
    if (!strcmp (av[0], "local")) {
        inst_loc(gd);
        count++;
    } else if (!strcmp (av[0], "clear")) {
        inst_clr(gd, 100000); /* built-in delay of 100ms */
        count++;
    } else if (!strcmp (av[0], "query")) {
        char rsp[MAX_RESPONSE_LEN];
        if (ac < 2) {
            printf ("query requires an argument\n");
        } else {
            (void)inst_qrystr(gd, av[1], rsp, sizeof(rsp));
            printf("%s\n", rsp);
        }
        count += 2;
    } else if (!strcmp (av[0], "write")) {
        if (ac < 2) {
            printf ("write requires an argument\n");
        } else {
            inst_wrtstr(gd, av[1]);
        }
        count += 2;
    } else if (!strcmp (av[0], "read") || !strcmp (av[0], "?")) {
        char rsp[MAX_RESPONSE_LEN];
        inst_rdstr(gd, rsp, sizeof(rsp));
        printf("%s\n", rsp);
        count++;
    } else if (!strcmp (av[0], "delay")) {
        if (ac < 2) {
            printf ("delay requires a seconds argument\n");
        } else {
            usleep((useconds_t)(strtod(av[1], NULL) * 1000000.0));
        }
        count += 2;
    } else if (!strcmp (av[0], "status")) {
        unsigned char status, mask;
        if (ac < 2) {
            printf ("status requires a mask argument\n");
        } else {
            mask = (unsigned char)strtoul(optarg, NULL, 0);
            (void)inst_rsp(gd, &status);
            printf("%hhu\n", status);
            if ((status & mask) != 0)
                exit(1);
        }
        count += 2;
    } else if (!strcmp (av[0], "help")) {
        printf ("Command help:\n"
                "    <command>     write gpib message <command>\n"
                "    <query?>      write <command>, read response\n"
                "    quit/exit     exit shell\n"
                "    clear         send gpib clear command\n"
                "    local         send gpib local command\n"
                "    status MASK   read gpib status, exit if MASK matches\n"
                "    read/?        read gpib message\n"
                "    write MESSAGE write gpib message\n"
                "    query MESSAGE write gpib message, then read\n"
                "    delay SEC     delay SEC seconds\n"
                "    help          dispaly this help\n");
        count++;
    } else if (av[0][strlen (av[0]) - 1] == '?') {
        char rsp[MAX_RESPONSE_LEN];
        (void)inst_qrystr(gd, av[0], rsp, sizeof(rsp));
        printf("%s\n", rsp);
        count++;
    } else {
        inst_wrtstr(gd, av[0]);
        count++;
    }
    return count;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
