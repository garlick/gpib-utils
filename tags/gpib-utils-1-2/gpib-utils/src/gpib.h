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

typedef void (*srqfun_t)(int d);

int gpib_init(char *progname, char *instrument, int vflag);

int gpib_ibrd(int d, void *buf, int len);
void gpib_ibrdstr(int d, char *buf, int len);

void gpib_ibwrt(int d, void *buf, int len);
void gpib_ibwrtstr(int d, char *str);
void gpib_ibwrtf(int d, char *fmt, ...);

void gpib_ibloc(int d);
void gpib_ibclr(int d);
void gpib_ibtrg(int d);
void gpib_ibrsp(int d, char *status);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
