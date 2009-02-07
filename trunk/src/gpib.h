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

typedef struct gpib_device *gd_t;

/* Callback function to interpret results of serial poll.
 * Return value: -1=!ready, 0=success, >0=fatal error
 */
typedef int (*spollfun_t)(gd_t gd, unsigned char status_byte, char *msg);

/* Look up a device's default address in /etc/gpib-utils.conf
 * Result is allocated in static storage that may be overwritten on next call.
 */
char *gpib_default_addr(char *name);

/* Initialize/finalize a device.  If sf is non-NULL, a serial poll is 
 * run after every I/O and the resulting status byte is passed to the sf
 * function for processing.  In case serial poll returns not ready, 'retry' 
 * is a backoff factor (in usecs) to sleep before retrying.
 */
gd_t gpib_init(char *addr, spollfun_t sf, unsigned long retry);
void gpib_fini(gd_t gd);

/* Set the gpib timeout in seconds.
 */
void gpib_set_timeout(gd_t gd, double sec);

/* Set flag that determines whether "telemetry" is displayed on
 * stderr as reads/writes are processed on the gpib.
 */
void gpib_set_verbose(gd_t gd, int flag);

/* Set flag that determines whether EOI is set on the last byte
 * of every write.
 */
void gpib_set_eot(gd_t gd, int flag);

/* Set flag that determines whether reads are terminated when
 * end-of-string is read.
 */
void gpib_set_reos(gd_t gd, int flag);

/* Set character to be used as end-of-string terminator.
 */
void gpib_set_eos(gd_t gd, int c);

int gpib_rd(gd_t gd, void *buf, int len);
void gpib_rdstr(gd_t gd, char *buf, int len);
int gpib_rdf(gd_t gd, char *fmt, ...);

void gpib_wrt(gd_t gd, void *buf, int len);
void gpib_wrtstr(gd_t gd, char *str);
void gpib_wrtf(gd_t gd, char *fmt, ...);

int gpib_qry(gd_t gd, char *str, void *buf, int len);
int gpib_qryint(gd_t gd, char *str);
int gpib_qrystr(gd_t gd, char *str, char *buf, int len);

void gpib_loc(gd_t gd);
void gpib_clr(gd_t gd, unsigned long usec);
void gpib_trg(gd_t gd);
int gpib_rsp(gd_t gd, unsigned char *status);

void gpib_abort(gd_t gd); /* VXI only */

typedef struct {
    char *sopt;
    char *lopt;
    char *desc;
} opt_desc_t;

void usage(opt_desc_t *tab);

#define OPTIONS_COMMON "a:lv"
#define OPTIONS_COMMON_LONG \
    {"address",         required_argument, 0, 'a'}, \
    {"local",           no_argument,       0, 'l'}, \
    {"verbose",         no_argument,       0, 'v'}

#define OPTIONS_COMMON_DESC \
    {"a", "address",   "set instrument address" }, \
    {"l", "local",     "return instrument to front panel control" }, \
    {"v", "verbose",   "show protocol on stderr" }

#define OPTIONS_COMMON_SWITCH \
    case 'a': \
    case 'l': \
    case 'v':

gd_t gpib_init_args(int argc, char *argv[], const char *opts, 
                    struct option *longopts, char *name, 
                    spollfun_t sf, unsigned long retry, int *opt_error);

/* 488.2 functions */

#define GPIB_DECODE_DLAB    1
#define GPIB_DECODE_ILAB    2
#define GPIB_VERIFY_DLAB    4
#define GPIB_VERIFY_ILAB    8
int gpib_decode_488_2_data(unsigned char *data, int *lenp, int flags);

/* required status reporting common commands */
void gpib_488_2_cls(gd_t gd);
int gpib_488_2_ese(gd_t gd);
int gpib_488_2_esr(gd_t gd);
int gpib_488_2_sre(gd_t gd);
int gpib_488_2_stb(gd_t gd);

/* required internal operation common commands */
void gpib_488_2_idn(gd_t gd);
void gpib_488_2_rst(gd_t gd, int delay_secs);
void gpib_488_2_tst(gd_t gd, strtab_t *tab);

/* optional learn command (and restore helper function) */
void gpib_488_2_lrn(gd_t gd);
void gpib_488_2_restore(gd_t gd);

/* optional option identification command */
void gpib_488_2_opt(gd_t gd);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
