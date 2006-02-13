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

#include <unistd.h>
#include <errno.h>

int
read_all(int fd, unsigned char *buf, int len)
{
    int n;
    int bc = 0;

    do {
        do {
            n = read(fd, buf + bc, len - bc);
        } while (n < 0 && errno == EINTR);
        if (n > 0)
            bc += n;
    } while (n > 0 && bc < len);

    return bc;
}

int
write_all(int fd, unsigned char *buf, int len)
{
    int n;
    int bc = 0;

    do {
        do {
            n = write(fd, buf + bc, len - bc);
        } while (n < 0 && errno == EINTR);
        if (n > 0)
            bc += n;
    } while (n > 0 && bc < len);

    return bc;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
