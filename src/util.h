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

int read_all(int fd, void *buf, int count);
int write_all(int fd, void *buf, int count);

void *xmalloc(size_t size);
char *xstrdup(char *str);
void *xrealloc(void *ptr, int size);

char *xstrcpyprint(char *str);
char *xstrcpyunprint(char *str); 

typedef struct {
   int     num;
   char   *str;
} strtab_t;

char *findstr(strtab_t *tab, int num);
int rfindstr(strtab_t *tab, char *str);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */