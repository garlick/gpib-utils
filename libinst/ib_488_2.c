/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.
  
   Copyright (C) 2001-2009 Jim Garlick <garlick.jim@gmail.com>
  
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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "gpib.h"
#include "ib_488_2.h"

extern char *prog;

/* 
 * Verify/decode some 488.2 data formats.
 */
static int
_extract_dlab_len(unsigned char *data, int lenlen, int len)
{
    char tmpstr[64];
  
    if (lenlen > len || lenlen > sizeof(tmpstr) - 1)
       return -1;
    memcpy(tmpstr, data, lenlen);
    tmpstr[lenlen] = '\0';
    return strtoul(tmpstr, NULL, 10);
}

int
gpib_decode_488_2_data(unsigned char *data, int *lenp, int flags)
{
    int len = *lenp;

    if (len < 3)
        return -1;

    /* 8.7.10 indefinite length arbitrary block response data */
    if (data[0] == '#' && data[1] == '0' && data[len - 1] == '\n') {
        if (!(flags & GPIB_DECODE_ILAB) && !(flags & GPIB_VERIFY_ILAB)) {
            fprintf(stderr, "%s: unexpectedly got ILAB response\n", prog);
            return -1;
        }
        if (flags & GPIB_DECODE_ILAB) {
            memmove(data, &data[2], len - 3);
            *lenp -= 3;
        }

    /* 8.7.9 definite length arbitrary block response data */
    } else if (data[0] == '#' && data[1] >= '1' && data[1] <= '9') {
        int llen = data[1] - '0';
        int dlen = _extract_dlab_len(&data[2], llen, len - 2);

        if (!(flags & GPIB_DECODE_DLAB) && !(flags & GPIB_VERIFY_DLAB)) {
            fprintf(stderr, "%s: unexpectedly got DLAB response\n", prog);
            return -1;
        }
        dlen = _extract_dlab_len(&data[2], data[1] - '0', len - 2);
        if (dlen == -1) {
            fprintf(stderr, "%s: failed to decode DLAB data length\n", prog);
            return -1;
        }
        if (dlen + 2 + llen > len) {
            fprintf(stderr, "%s: DLAB data length is too large\n", prog);
            return -1;
        }
        if (flags & GPIB_DECODE_DLAB) {
            memmove(data, data + 2 + llen, dlen);
            *lenp = dlen;
        }
    } else {
        fprintf(stderr, "%s: unexpected or garbled 488.2 response\n", prog);
        return -1;
    }

    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

/* 
 * 488.2 common commands 
 */

void
gpib_488_2_cls(gd_t gd)
{
    gpib_wrtstr(gd, "*CLS\n");
}

void
gpib_488_2_rst(gd_t gd, int delay_secs)
{
    gpib_wrtstr(gd, "*RST\n");
    sleep(delay_secs);
}

void
gpib_488_2_idn(gd_t gd)
{
    char buf[128];

    gpib_qrystr(gd, "*IDN?\n", buf, sizeof(buf));
    printf("%s: idn-string: %s\n", prog, buf);
}

void
gpib_488_2_opt(gd_t gd)
{
    char buf[128];

    gpib_qrystr(gd, "*OPT?\n", buf, sizeof(buf));
    printf("%s: installed-options: %s\n", prog, buf);
}

void
gpib_488_2_tst(gd_t gd, strtab_t *tab)
{
    int result = gpib_qryint(gd, "*TST?\n");

    if (result == 0)
        printf("self-test: passed\n");
    else if (tab)
        printf("self-test: failed: %s\n", findstr(tab, result));
    else
        printf("self-test: failed (code %d)\n", result);
}

void
gpib_488_2_lrn(gd_t gd)
{
    char buf[32768];
    int len;

    len = gpib_qrystr(gd, "*LRN?\n", buf, sizeof(buf));
    if (write_all(1, buf, len) < 0) {
        fprintf(stderr, "%s: write error on stdout: %s\n",
                prog, strerror(errno));
        exit(1);
    }
}

void
gpib_488_2_restore(gd_t gd)
{
    char buf[32768];
    int len;

    if ((len = read_all(0, buf, sizeof(buf)) < 0)) {
        fprintf(stderr, "%s: read error on stdin: %s\n",
                prog, strerror(errno));
        exit(1);
    }
    gpib_wrt(gd, buf, len);
    fprintf(stderr, "%s: restore setup: %d bytes\n", prog, len);
}

int
gpib_488_2_stb(gd_t gd)
{
    return gpib_qryint(gd, "*STB?\n");
}

int
gpib_488_2_esr(gd_t gd)
{
    return gpib_qryint(gd, "*ESR?\n");
}

int
gpib_488_2_ese(gd_t gd)
{
    return gpib_qryint(gd, "*ESE?\n");
}

int
gpib_488_2_sre(gd_t gd)
{
    return gpib_qryint(gd, "*SRE?\n");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
