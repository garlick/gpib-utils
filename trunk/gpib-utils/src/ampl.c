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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <errno.h>

static int
_vtodb(double *a, double offset)
{
    int result = 0;
    double l = log10(*a);

    if (l == EDOM || l == ERANGE || isinf(l) || isnan(l))
        result = -1;
    else
        *a = l*20.0 + offset;

    return result;
}

/* Set *ampl to amplitutde in dBm.
 * Return 0 on success, -1 on failure.
 *
 * See pg 64. of HP-8656A operator manual for conversions used below.
 */
int
amplstr(char *str, double *ampl)
{
    int result = 0;
    char *end;
    double a = strtod(str, &end);

    if (!*end)  /* no units */
        result = -1;
    else if (!strcasecmp(end, "dbm"))
        ;
    else if (!strcasecmp(end, "dbf"))
        a -= 120.0;
    else if (!strcasecmp(end, "dbv"))
        a += 13.0;
    else if (!strcasecmp(end, "dbmv"))
        a -= 47.0;
    else if (!strcasecmp(end, "dbuv"))
        a -= 107.0;
    else if (!strcasecmp(end, "dbemfv"))
        a += 7.0;
    else if (!strcasecmp(end, "dbemfmv"))
        a -= 53.0;
    else if (!strcasecmp(end, "dbemfuv"))
        a -= 113.0;
    else if (!strcasecmp(end, "v"))
        result = _vtodb(&a, 13.0);
    else if (!strcasecmp(end, "mv"))
        result = _vtodb(&a, -47.0);
    else if (!strcasecmp(end, "uv"))
        result = _vtodb(&a, -107.0);
    else if (!strcasecmp(end, "emfv"))
        result = _vtodb(&a, 7.0);
    else if (!strcasecmp(end, "emfmv"))
        result = _vtodb(&a, -53.0);
    else if (!strcasecmp(end, "emfmuv"))
        result = _vtodb(&a, -113.0);
    else  
        result = -1;
    if (result == 0 && ampl != NULL)
        *ampl = a;
    return result;
}

#ifdef TESTMAIN
int
main(int argc, char *argv[])
{
    int i;
    double a;

    for (i = 1; i < argc; i++) {
        if (amplstr(argv[i], &a) < 0)
            printf("error\n");
        else
            printf("%s = %e dBm (%e V)\n", argv[i], a, pow(10, (a-13.0)/20));
    }
    exit(0);
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
