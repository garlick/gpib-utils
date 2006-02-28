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

/* Convert HP 1630/31 state/timing trace dumps to dinotrace (DECSIM text) 
 * trace format.  HP dump file on stdin, decsim text on stdout.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>

#include "hp1630.h"
#include "units.h"
#include "hpcrc.h"

struct pods {
    uint16_t pod0; /* 8 bit state/timing */
    uint16_t pod1; /* 8 bit state/timing */
    uint16_t pod2; /* 9 bit state */
    uint16_t pod3; /* 9 bit state */
    uint16_t pod4; /* 9 bit state */
};

char *prog = "<unknown>";
double clock_ns = 100; /* clock period in ns */

#define OPTIONS "c:"
static struct option longopts[] = {
    {"clock",       required_argument, 0, 'c'},
    {0, 0, 0, 0},
};

static uint16_t 
_getint16(uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8) + buf[1];
}

#if 0
static uint32_t 
_getint32(uint8_t *buf)
{
    return ((uint32_t)_getint16(&buf[0]) << 16) + _getint16(&buf[2]);
}
#endif

/* one byte contains two padded bcd digits */
static int 
_getint8bcd(uint8_t v)
{
    return (v & 0xf) + ((v >> 4) & 0xf) * 10;
}


static void
_parse_err(char *msg)
{
    fprintf(stderr, "%s: parse error on stdin: %s\n", prog, msg);
    exit(1);
}

static time_t 
_gettime(uint8_t *buf)
{
    struct tm tm;

    if ((tm.tm_mon = buf[0]) > 11)
        _parse_err("month out of range");     
    if ((tm.tm_mday = _getint8bcd(buf[1])) > 31)
        _parse_err("day out of range");     
    if ((tm.tm_hour = _getint8bcd(buf[2])) > 23)
        _parse_err("hour out of range");     
    if ((tm.tm_min = _getint8bcd(buf[3])) > 59)
        _parse_err("min out of range");     
    if ((tm.tm_sec = _getint8bcd(buf[4])) > 59)
        _parse_err("sec out of range");     
    tm.tm_year = _getint16(&buf[5]) - 1900;
    tm.tm_isdst = -1; /* info unavailable */

    return mktime(&tm); /* -1 on conversion failure */
}

static int
_read_learn_string(uint8_t *buf, int size)
{
    uint16_t crc;
    uint16_t len = 0;


    if (fread(&buf[0], 4, 1, stdin) == 1) {
        if (buf[0] != 'R')
            _parse_err("expected 'R'");
        len = _getint16(&buf[2]); /* bytes to follow */
        assert(len - 4 <= size);
        if (fread(&buf[4], len, 1, stdin) != 1) {
            fprintf(stderr, "%s: fread error\n", prog);
            exit(1);
        }
        len += 4; /* make len be length of whole learn string */
        crc = _getint16(&buf[len - 2]);
        if (hpcrc(buf+4, len-6) != crc) {
            fprintf(stderr, "%s: crc failure\n", prog);
            exit(1);
        }
    }

    return len;
}

static void
_printbin(uint16_t v, int bits)
{
    int i;

    for (i = bits - 1; i >= 0; i--)
        printf("%d", (v >> i) & 1);
}

static void
_parse_state_trace(uint8_t *buf, int len)
{
    time_t t = _gettime(&buf[4]);           /* date */
    int channels = buf[13];                 /* no. state channels */
    int states = _getint16(&buf[14]);       /* no. valid states */
    int tracepoint = _getint16(&buf[16]);   /* index of state tracepoint */
    int rev = buf[len - 3];                 /* firmware? revision */
    int i;
    int bpw = channels == 27 ? 4 :      /* bytes per word */
              channels == 35 ? 5 :
              channels == 43 ? 6 : -1;  /* XXX add 65 channels (hp1630g) */
    struct pods state[4096];
    int oh = len - (states * bpw);      /* non-trace data in learn string */

    if (bpw == -1) {
        fprintf(stderr, "%s: no support for %d channel mode\n", prog, channels);
        exit(1);
    }

    assert(oh == 21); 

    fprintf(stderr, "%s: state: %d channels, %d states, %s", prog, 
            channels, states, t == -1 ? "[invalid time stamp]\n" : ctime(&t));
    fprintf(stderr, "%s: state: tracepoint %d, rev %d\n", prog, 
            tracepoint, rev);

    /* XXX my hp1631a shows rev 241 */

    assert(states <= sizeof(state)/sizeof(state[0]));

    /* 
     * Copy state trace information into state[].
     */

    for (i = 0; i < states; i++) {
        int j = 18 + (i * bpw);

        /* This takes care of HP1630A/D, HP1631A/D, but not HP1630G */

        /* first byte[2:0] is pod4[8:6] */
        /* second byte[7:2] is pod4[5:0] */
        state[i].pod4 = ((uint16_t)(buf[j] & 7) << 6) 
                      | ((uint16_t)(buf[j+1] & 0xfc) >> 2);

        /* second byte[1:0] is pod3[8:7] */
        /* third byte[7:1] is pod3[6:0] */
        state[i].pod3 = ((uint16_t)(buf[j+1] & 3) << 7) 
                      | ((uint16_t)(buf[j+2] & 0xfe) >> 1);

        /* third byte[0] is pod2[8] */
        /* fourth byte[7:0] is pod2[7:0] */
        state[i].pod2 = ((uint16_t)(buf[j+2] & 1) << 8) | buf[j+3];

        /* (optional) fifth byte[7:0] is pod1[7:0] */
        if (bpw >= 5)
            state[i].pod1 = buf[j+4];

        /* (optional) sixth byte[7:0] is pod0[7:0] */
        if (bpw >= 6)
            state[i].pod0 = buf[j+5];
    }

    /* 
     * dinotrace dump
     */

    printf("!            PPPPPPPPPPPPPPPPPPPPPPPPPPP%s%s\n",
            bpw >= 5 ? "PPPPPPPP" : "", bpw >= 6 ? "PPPPPPPP" : "");
    printf("!            OOOOOOOOOOOOOOOOOOOOOOOOOOO%s%s\n",
            bpw >= 5 ? "OOOOOOOO" : "", bpw >= 6 ? "OOOOOOOO" : "");
    printf("!            DDDDDDDDDDDDDDDDDDDDDDDDDDD%s%s\n",
            bpw >= 5 ? "DDDDDDDD" : "", bpw >= 6 ? "DDDDDDDD" : "");
    printf("!            444444444333333333222222222%s%s\n",
            bpw >= 5 ? "11111111" : "", bpw >= 6 ? "00000000" : "");
    printf("!            ---------------------------%s%s\n",
            bpw >= 5 ? "--------" : "", bpw >= 6 ? "--------" : "");
    printf("!            876543210876543210876543210%s%s\n",
            bpw >= 5 ? "76543210" : "", bpw >= 6 ? "76543210" : "");

    for (i = 0; i < states; i++) {
        printf("%-12.12lu ", (unsigned long)clock_ns * i);
        _printbin(state[i].pod4, 9);
        _printbin(state[i].pod3, 9);
        _printbin(state[i].pod2, 9);
        if (bpw >= 5)
            _printbin(state[i].pod1, 8);
        if (bpw >= 6)
            _printbin(state[i].pod0, 8);
        printf("\n");
    }
}

static void
_parse_timing_trace(uint8_t *buf, int len)
{
    int channels = buf[4];              /* no. timing channels */
    int states = _getint16(&buf[5]);    /* no. valid timing states */
    int tracepoint = _getint16(&buf[7]);/* index of trace point state */
    int glitch = buf[9];                /* glitch data taken? */
    int period = _getint16(&buf[10]);   /* sample period */
    time_t t = _gettime(&buf[12]);      /* date */
    int trace_offset;
    int rev;
    int i;
    int oh = len - (states * channels/8); /* non-trace data in learn string */
    struct pods state[1024];

    /* after the date value, formats differ */
    switch (oh) {
        case 21: /* hp1630 manual */
            rev = 0; /* no rev available - go with 0  */
            trace_offset = oh - 2;
            break;
        case 62: /* hp1631 manual */
            rev = buf[len - 3];
            trace_offset = oh - 3;
            break;
        case 54: /* my hp1631a (perhaps older rev than manual?) */
            rev = buf[len - 3];
            trace_offset = oh - 3;
            break;
        default:
            fprintf(stderr, "%s: unsupported trace format\n", prog);
            exit(1);
    }

    assert(states <= sizeof(state)/sizeof(state[0]));
    assert(channels == 8 || channels == 16);

    fprintf(stderr, "%s: timing: %d channels, %d states, %s", prog, 
            channels, states, t == -1 ? "[invalid time stamp]\n" : ctime(&t));
    fprintf(stderr, "%s: timing: glitch %s, period %d\n", prog,
            glitch ? "on" : "off", period);
    fprintf(stderr, "%s: timing: tracepoint %d, rev %d\n", prog,
            tracepoint, rev);

    /* XXX my hp1631a shows rev 241 */

    /* XXX need to handle glitch data */
    assert(!glitch);

    /* 
     * Copy timing information into state[].
     */

    for (i = 0; i < states; i++) {
        int j = trace_offset + (i * channels/8);

        state[i].pod1 = buf[j];
        if (channels == 16)
            state[i].pod0 = buf[j+1];
    }

    /* 
     * Dinotrace dump.
     */

    printf("!            PPPPPPPP%s\n", channels == 16 ? "PPPPPPPP" : "");
    printf("!            OOOOOOOO%s\n", channels == 16 ? "OOOOOOOO" : "");
    printf("!            DDDDDDDD%s\n", channels == 16 ? "DDDDDDDD" : "");
    printf("!            11111111%s\n", channels == 16 ? "00000000" : "");
    printf("!            --------%s\n", channels == 16 ? "--------" : "");
    printf("!            76543210%s\n", channels == 16 ? "76543210" : "");

    /* XXX must figure out how to decode period.  Until then, use clock_ns. */
    for (i = 0; i < states; i++) {
        printf("%-12.12lu ", (unsigned long)clock_ns * i);
        _printbin(state[i].pod1, 8);
        if (channels == 16)
            _printbin(state[i].pod0, 8);
        printf("\n");
    }
}

static void
_parse_learn_string(uint8_t *buf, int len)
{
    switch(buf[1]) {
        case 'C': /* config */
            break;
        case 'S': /* state */
            switch (buf[12]) {
                case 0: /* trace data */
                    _parse_state_trace(buf, len);
                    break;
                case 1: /* label overview */
                    break;
                case 2: /* time interval overview */
                    break;
                default:
                    _parse_err("unknown state learn string");
            }
            break;
        case 'T': /* timing data */
            _parse_timing_trace(buf, len);
            break;
        case 'A': /* analog */
            break;
        default:
            _parse_err("unknown learn string");
            break; 
    }
}

static void
_usage(void)
{
    fprintf(stderr, "Usage: %s [--clock freqstr] <state.dump >state.tra\n", 
            prog);
    exit(1);
}

int
main(int argc, char *argv[])
{
    int len;
    uint8_t buf[16*1024];
    int c;
    double f;

    /* XXX If state and timing learn strings are both on stdin, we output 
     * a concatenated trace which is wrong.  Need to figure out how to make 
     * properly correlated state/timing trace output.
     */

    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
            case 'c':   /* --clock */
                if (freqstr(optarg, &f) < 0) {
                    fprintf(stderr, "%s: freq conversion error\n", prog);
                    fprintf(stderr, "%s: use freq units: %s\n", prog, FREQ_UNITS);
                    fprintf(stderr, "%s: or period units: %s\n", prog,PERIOD_UNITS);
                    exit(1);
                }
                clock_ns = 1000000000.0/f;
                break;
            default:
                _usage();
        }
    }

    /* XXX We use this for timing trace even though in theory we have
     * the sample interval in the learn string.  
     * See note in _parse_timing_trace().
     */

    fprintf(stderr, "%s: using %.1fns clock\n", prog, clock_ns);

    do {
        len = _read_learn_string(buf, sizeof(buf));
        if (len > 0)
            _parse_learn_string(buf, len);
    } while (len > 0);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
