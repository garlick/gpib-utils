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

#include <stdlib.h>
#include <strings.h>

/* Set *freq to frequency in Hz.
 * Return 0 on success, -1 on failure.
 */
int
freqstr(char *str, double *freq)
{
    char *end;
    double f = strtod(str, &end);
    int result = 0;

    if (!*end)  /* no units is an error */
        result = -1;
    else if (!strcasecmp(end, "hz"))
        ;
    else if (!strcasecmp(end, "khz"))
        f *= 1E3;
    else if (!strcasecmp(end, "mhz"))
        f *= 1E6;
    else if (!strcasecmp(end, "ghz"))
        f *= 1E9;
    else if (!strcasecmp(end, "sec") || !strcasecmp(end, "s"))
        f = 1.0/f;
    else if (!strcasecmp(end, "msec") || !strcasecmp(end, "ms"))
        f = 1E3/f;
    else if (!strcasecmp(end, "usec") || !strcasecmp(end, "us"))
        f = 1E6/f;
    else if (!strcasecmp(end, "nsec") || !strcasecmp(end, "ns"))
        f = 1E9/f;
    else if (!strcasecmp(end, "psec") || !strcasecmp(end, "ps"))
        f = 1E12/f;
    else  
        result = -1;

    if (result == 0 && freq != NULL)
        *freq = f;

    return result;
}

#ifdef TESTMAIN
int
main(int argc, char *argv[])
{
    int i;
    double f;

    for (i = 1; i < argc; i++) {
        if (freqstr(argv[i], &f) < 0)
            printf("error\n");
        else
            printf("%s = %e hz\n", argv[i], f);
    }
    exit(0);
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
