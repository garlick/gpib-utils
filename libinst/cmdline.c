/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.
  
   Copyright (C) 2001-2009 Jim Garlick <garlick.jim@gmail.com>
  
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
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <wordexp.h>
#include <libgen.h>

#include "gpib.h"
#include "cmdline.h"

extern char *prog;

static char *
_parse_line(char *buf, char *key)
{
    char *k;

    for (k = buf; *k != '\0'; k++)
        if (*k == '#')
            *k = '\0';
    k = strtok(buf, " \t\r\n");
    if (!k || strcmp(k, key) != 0)
        return NULL;
    return strtok(NULL, " \t\r\n");
}

char *
gpib_default_addr(char *name)
{
    char *paths[] = GPIB_UTILS_CONF, **p = &paths[0];
    wordexp_t w;
    FILE *cf = NULL;
    static char buf[64];
    char *res = NULL;

    while (cf == NULL && *p != NULL) {
        if (wordexp(*p++, &w, 0) == 0 && w.we_wordc > 0)
             cf = fopen(w.we_wordv[0], "r");
        wordfree(&w);
    }
    if (cf) {
        while (res == NULL && fgets(buf, sizeof(buf), cf) != NULL)
            res = _parse_line(buf, name);
        (void)fclose(cf);
    }
    return res;
}

/* Handle common -v and -a option processing in each of the utilities,
 * and initialize the GPIB connection.
 */
gd_t
gpib_init_args(int argc, char *argv[], const char *opts, 
               struct option *longopts, char *name, spollfun_t sf, 
               unsigned long retry, int *opt_error)
{
    int c, verbose = 0, todo = 0;
    char *addr = NULL;
    gd_t gd = NULL;
    int error = 0;

    prog = basename(argv[0]);
    while ((c = getopt_long (argc, argv, opts, longopts, NULL)) != EOF) {
        switch (c) {
            case 'a': /* --address */
                addr = optarg;
                break;
            case 'v': /* --verbose */
                verbose = 1;
                break;
            case 'n': /* --name */
                name = optarg;
                break;
            case '?':
                error++;
                break;
            default:
                todo++;
                break;
        }
    }
    if (error || !todo || optind < argc) {
        *opt_error = 1;
        goto done;
    }
    *opt_error = 0;

    if (!addr) {
        if (name) {
            addr = gpib_default_addr(name);
            if (!addr) {
                fprintf(stderr, "%s: no dflt address for %s, use --address\n", 
                        prog, name);
                goto done;
            }
        } else {
            fprintf(stderr, "%s: --name or --address are required\n", prog);
            goto done;
        }
    }
    gd = gpib_init(addr, sf, 0);
    if (!gd) {
        fprintf(stderr, "%s: device initialization failed for address %s\n", 
                prog, addr);
        goto done;
    }
    gpib_set_verbose(gd, verbose);
done:
    optind = 0;
    return gd;
}

void 
usage(opt_desc_t *tab)
{
    int i;
    int width = 23;
    int fw;

    printf("Usage: %s [--options]\n", prog);
    for (i = 0; (tab[i].sopt && tab[i].lopt && tab[i].desc); i++) {
        if (strlen(tab[i].lopt) + strlen(tab[i].sopt) + 4 > width)
            printf("  -%s,--%s\n  %-*s", tab[i].sopt, tab[i].lopt, width, "");
        else {
            fw = width - 4 - strlen(tab[i].sopt);
            printf("  -%s,--%-*s", tab[i].sopt, fw, tab[i].lopt);
        }
        printf(" %s\n", tab[i].desc);
    }
    exit(1);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
