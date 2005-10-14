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

double 
freqstr(char *str)
{
    char *end;
    double f = strtod(str, &end);

    if (!*end)
        f = -1; /* error */
    else if (!strcasecmp(end, "hz"))
        ;
    else if (!strcasecmp(end, "khz"))
        f *= 1000;
    else if (!strcasecmp(end, "mhz"))
        f *= 1000000;
    else if (!strcasecmp(end, "ghz"))
        f *= 1000000000;
    else if (!strcasecmp(end, "sec") || !strcasecmp(end, "s"))
        f = 1.0/f;
    else if (!strcasecmp(end, "msec") || !strcasecmp(end, "ms"))
        f = 1000.0/f;
    else if (!strcasecmp(end, "usec") || !strcasecmp(end, "us"))
        f = 1000000.0/f;
    else if (!strcasecmp(end, "nsec") || !strcasecmp(end, "ns"))
        f = 1000000000.0/f;
    else if (!strcasecmp(end, "psec") || !strcasecmp(end, "ps"))
        f = 1000000000000.0/f;
    else  
        f = -1; /* error */
    return f;
}

#if 0
void
main(int argc, char *argv[])
{
    int i;
    double f;

    for (i = 1; i < argc; i++) {
        f = freqstr(argv[i]);
        printf("%s = %e hz\n", argv[i], f);
    }
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
