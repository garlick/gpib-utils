/* thello.c - read vxi11 instrument identification */

/* Simplest VXI-11 program to validate shared library linkage.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#if HAVE_STDBOOL_H
#include <stdbool.h>
#else
typedef enum { false=0, true=1 } bool;
#endif

#include <vxi11_device.h>

void
usage (void)
{
    fprintf (stderr, "Usage: thello hostname:inst0\n");
    exit (1);
}

int
main (int argc, char *argv[])
{
    vxi11dev_t v;
    int err;
    char buf[80];

    if (argc != 2) {
        usage ();
    }

    if (!(v = vxi11_create ())) {
        fprintf (stderr, "out of memory");
        exit (1);
    }
    if ((err = vxi11_open (v, argv[1], false)) != 0) {
        vxi11_perror (v, err, "vxi11_open");
        exit (1);
    }
    if ((err = vxi11_write (v, "*IDN?", 5)) != 0) {
        vxi11_perror (v, err, "vxi11_write");
        exit (1);
    }
    if ((err = vxi11_readstr (v, buf, sizeof (buf))) != 0) {
        vxi11_perror (v, err, "vxi11_readstr");
        exit (1);
    }
    printf ("%s", buf);
    vxi11_close (v);
    vxi11_destroy (v);
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

