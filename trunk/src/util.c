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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "util.h"

extern char *prog;

int
read_all(int fd, void *buf, int len)
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
write_all(int fd, void *buf, int len)
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

void *
xmalloc(size_t size)
{
    void *new = malloc(size);

    if (!new) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    return new;
}

void *
xrealloc(void *ptr, int size)
{
    void *new = realloc(ptr, size);

    if (!new) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    return new;
}

char *
xstrdup(char *str)
{
    char *new = strdup(str);

    if (!new) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    return new;
}

char *
xstrcpyprint(char *str)
{
    char *cpy = xmalloc(strlen(str)*2 + 1);
    int i;

    *cpy = '\0';
    for (i = 0; i < strlen(str); i++) {
        if (str[i] == '\n')
            strcat(cpy, "\\n");
        else if (str[i] == '\r')
            strcat(cpy, "\\r");
        else
            strncat(cpy, &str[i], 1);
    }
    return cpy;
}

char *
xstrcpyunprint(char *str)
{
    char *cpy = xmalloc(strlen(str) + 1);
    int i;
    int esc = 0;

    *cpy = '\0';
    for (i = 0; i < strlen(str); i++) {
        if (esc) {
            switch (str[i]) {
                case 'r':
                    strcat(cpy, "\r");
                    break;
                case 'n':
                    strcat(cpy, "\n");
                    break;
                case '\\':
                    strcat(cpy, "\\");
                    break;
                default:
                    strcat(cpy, "\\");
                    strncat(cpy, &str[i], 1);
                    break;
            }
            esc = 0;
        } else if (str[i] == '\\') {
            esc = 1;
        } else {
            strncat(cpy, &str[i], 1);
        }
    }

    return cpy;
}

char *
findstr(strtab_t *tab, int num)
{
    strtab_t *tp;
    static char tmpbuf[64];
    char *res = tmpbuf;

    (void)sprintf(tmpbuf, "unknown code %d", num);
    for (tp = &tab[0]; tp->str; tp++) {
        if (tp->num == num) {
            res = tp->str;
            break;
        }
    }
    return res;
}

int
rfindstr(strtab_t *tab, char *str)
{
    strtab_t *tp;
    int res = -1;

    for (tp = &tab[0]; tp->str; tp++) {
        if (!strcasecmp(tp->str, str)) {
            res = tp->num;
            break;
        }
    }
    return res;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
