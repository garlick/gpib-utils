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


/* Set *ampl to amplitutde in dBm and return 0 on success, -1 on failure.
 * Note: 0 dBm is voltage corresponding to 1mW into some load impedance.
 * at 50 ohms, 0 dBm is 0.22V rms.  Ref: "The Art of Electronics", 2ed by
 * Horowitz and Hill.  See pg 64. of HP-8656A operator manual for conversions 
 * used below.
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
        a = log10(a)*20.0 + 13.0;
    else if (!strcasecmp(end, "mv"))
        a = log10(a)*20.0 - 47.0;
    else if (!strcasecmp(end, "uv"))
        a = log10(a)*20.0 - 107.0;
    else if (!strcasecmp(end, "emfv"))
        a = log10(a)*20.0 + 7.0;
    else if (!strcasecmp(end, "emfmv"))
        a = log10(a)*20.0 - 53.0;
    else if (!strcasecmp(end, "emfmuv"))
        a = log10(a)*20.0 - 113.0;
    else  
        result = -1;
    if (result == 0 && ampl != NULL)
        *ampl = a;
    return result;
}

#if 0
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
