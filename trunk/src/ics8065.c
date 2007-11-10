#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "errstr.h"
#include "ics.h"



#define OPTIONS "n:resgiIj:cfC"
static struct option longopts[] = {
        {"name",                required_argument,  0, 'n'},
        {"get-interface-name",  no_argument,        0, 'I'},
        {"set-interface-name",  required_argument,  0, 'j'},
        {"reload-config",       no_argument,        0, 'C'},
        {"reload-factory",      no_argument,        0, 'f'},
        {"commit-config",       no_argument,        0, 'c'},
        {"reboot",              no_argument,        0, 'r'},
        {"get-idn-string",      no_argument,        0, 'i'},
        {"errlog",              no_argument,        0, 'e'},
        {0, 0, 0, 0}
};

static errstr_t errtab[] = ICS_ERRLOG;

char *prog;

void
usage(void)
{
    fprintf(stderr, 
  "Usage: %s [--options]\n"
  "  -n,--name                 instrument hostname (default: gpibgw)\n"
  "  -I,--get-interface-name   get VXI-11 logical name (e.g. gpib0)\n"
  "  -j,--set-interface-name   set VXI-11 logical name\n"
  "  -C,--reload-config        force reload of default config\n"
  "  -f,--reload-factory       reload factory config settings\n"
  "  -c,--commit-config        commit (write) current config\n"
  "  -r,--reboot               reboot\n"
  "  -i,--get-idn-string       get idn string\n"
  "  -e,--get-errlog           get error log (side effect: log is cleared)\n"
                       , prog);
        exit(1);

}

int
main(int argc, char *argv[])
{
    ics_t ics;
    int c;
    char *name = "gpibgw";
    int get_interface_name = 0;
    char *set_interface_name = NULL;
    int reload_config = 0;
    int reload_factory = 0;
    int commit_config = 0;
    int reboot = 0;
    int get_idn = 0;
    int get_errlog = 0;

    prog = basename(argv[0]);

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'n' :  /* --name */
                name = optarg;
                break;
            case 'I' :  /* --get-interface-name */
                get_interface_name = 1;
                break;
            case 'j' :  /* --set-interface-name */
                set_interface_name = optarg;
                break;
            case 'C':   /* --reload-config */
                reload_config = 1;
                break;
            case 'f':   /* --reload-factory */
                reload_factory = 1;
                break;
            case 'c':   /* --commit-config */
                commit_config = 1;
                break;
            case 'r' :  /* --reboot */
                reboot = 1;
                break;
            case 'i' :  /* --idn */
                get_idn = 1;
                break;
            case 'e' :  /* --errlog */
                get_errlog = 1;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();

    if(!get_errlog && !get_idn && !reboot 
            && !commit_config && !reload_factory && !reload_config
            && !get_interface_name && !set_interface_name)
        usage();

    ics = ics_init(name);
    if (!ics)
        exit(1);

    if (get_idn) {
        char *tmpstr;

        if (ics_idn_string(ics, &tmpstr) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
    }
    if (get_interface_name) {
        char *tmpstr;

        if (ics_get_interface_name(ics, &tmpstr) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
        free(tmpstr);
    }
    if (set_interface_name) {
        if (ics_set_interface_name(ics, set_interface_name) != 0)
            goto done;
    }
    if (reload_factory) {
        if (ics_reload_factory(ics) != 0)
            goto done;
        fprintf(stderr, "%s: factory config reloaded\n", prog);
    }
    if (reload_config) {
        if (ics_reload_config(ics) != 0)
            goto done;
        fprintf(stderr, "%s: default config reloaded\n", prog);
    }
    if (commit_config) {
        if (ics_commit_config(ics) != 0)
            goto done;
        fprintf(stderr, "%s: current config written\n", prog);
    }
    if (get_errlog) {
        unsigned int *tmperr;
        int i, tmpcount = 0;

        if (ics_error_logger(ics, &tmperr, &tmpcount) != 0)
            goto done;            
        for (i = 0; i < tmpcount ; i++)
            fprintf(stderr, "%s: %s\n", prog, finderr(errtab, tmperr[i]));
        free(tmperr);
    }

    if (reboot) {
        if (ics_reboot(ics) != 0)
            goto done;
        fprintf(stderr, "%s: rebooting\n", prog);
    }

done:
    ics_fini(ics);
    exit(0);
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

