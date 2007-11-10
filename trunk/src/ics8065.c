#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "errstr.h"
#include "ics.h"



#define OPTIONS "n:rhei"
static struct option longopts[] = {
        {"name",      required_argument,  0, 'n'},
        {"errlog",    no_argument,        0, 'e'},
        {"reboot",    no_argument,        0, 'r'},
        {"idn",       no_argument,        0, 'i'},
        {"help",      no_argument,        0, 'h'},
        {0, 0, 0, 0}
};

static errstr_t errtab[] = ICS_ERRLOG;

char *prog;

void
usage(void)
{
    fprintf(stderr, 
  "Usage: %s [--options]\n"
  "  -n,--name          hostname\n"
  "  -r,--reboot        reboot\n"
  "  -e,--errlog        fetch error log\n"
  "  -i,--idn           fetch and print idn string\n"
                       , prog);
        exit(1);

}

int
main(int argc, char *argv[])
{
    ics_t ics;
    int c;
    char *name = "gpibgw";
    int reboot = 0;
    int errlog = 0;
    int idn = 0;

    prog = basename(argv[0]);

    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'n' :  /* --name */
                name = optarg;
                break;
            case 'r' :  /* --reboot */
                reboot = 1;
                break;
            case 'e' :  /* --errlog */
                errlog = 1;
                break;
            case 'i' :  /* --idn */
                idn = 1;
                break;
            default:
                usage();
                break;
        }
    }
    if (optind < argc)
        usage();

    if(!errlog && !reboot && !idn)
        usage();

    ics = ics_init(name);
    if (!ics)
        exit(1);

    if (idn) {
        char tmpstr[128];

        if (ics_idnreply(ics, tmpstr, sizeof(tmpstr)) != 0)
            goto done;
        fprintf(stderr, "%s: %s\n", prog, tmpstr);
    }

    if (errlog) {
        unsigned int errors[100];
        int len = 0;
        int i;

        if (ics_errorlogger(ics, errors, 100, &len) != 0)
            goto done;            
        for (i = 0; i < len; i++)
            fprintf(stderr, "%s: %s\n", prog, finderr(errtab, errors[i]));
    }

    if (reboot) {
        fprintf(stderr, "%s: rebooting\n", prog);
        ics_reboot(ics);
    }

done:
    ics_fini(ics);
    exit(0);
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

