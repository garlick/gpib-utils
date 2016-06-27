/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005-2009 Jim Garlick <garlick.jim@gmail.com>

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

char *xreadline(char *prompt, char *buf, int buflen);

void *xmalloc(size_t size);
void *xzmalloc(size_t size);
char *xstrdup(const char *str);
void *xrealloc(void *ptr, int size);

char *xstrcpyprint(char *str);
char *xstrcpyunprint(char *str);

typedef struct {
   int     num;
   char   *str;
} strtab_t;

char *findstr(strtab_t *tab, int num);
int rfindstr(strtab_t *tab, char *str);

int     amplstr(char *str, double *ampl);
double  dbmtov(double a);

int     freqstr(char *str, double *freq);

#define FREQ_UNITS      "hz, khz, mhz, ghz"
#define PERIOD_UNITS    "s, ms, us, ns, ps"
#define AMPL_LOG_UNITS  "dbm, dbf, dbv, dbmv, dbuv, dbemfv, dbemfmv, dbemfuv"
#define AMPL_LIN_UNITS  "v, mv, uv, emfv, emfmv, emfuv"

double gettime(void);
void sleep_sec(double sec);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
