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
 * function for processing.  If the function returns "not ready", sleep 
 * 'sec' seconds before retrying the serial poll.
 */
gd_t gpib_init(char *addr, spollfun_t sf, unsigned long retry);
void gpib_fini(gd_t gd);

/* Get/set the gpib timeout in seconds.
 */
double gpib_get_timeout(gd_t gd);
void gpib_set_timeout(gd_t gd, double sec);

/* Get/set flag that determines whether "telemetry" is displayed on
 * stderr as reads/writes are processed on the gpib.
 */
int gpib_get_verbose(gd_t gd);
void gpib_set_verbose(gd_t gd, int flag);

/* Get/set flag that determines whether EOI is set on the last byte
 * of every write.
 */
int gpib_get_eot(gd_t gd);
void gpib_set_eot(gd_t gd, int flag);

/* Get/set flag that determines whether reads are terminated when
 * end-of-string is read.
 */
int gpib_get_reos(gd_t gd);
void gpib_set_reos(gd_t gd, int flag);

/* Get/set flag that determines whether EOI is transmitted whenever
 * end-of-string character is sent.
 */
int gpib_get_xeos(gd_t gd);
void gpib_set_xeos(gd_t gd, int flag);

/* Get/set flag that determines whether 7 (false) or 8 (true) bits 
 * are compared when looking for end-of-string character.
 */
int gpib_get_bin(gd_t gd);
void gpib_set_bin(gd_t gd, int flag);

/* Get/set character to be used as end-of-string terminator.
 */
int gpib_get_eos(gd_t gd);
void gpib_set_eos(gd_t gd, int c);

int gpib_rd(gd_t gd, void *buf, int len);
void gpib_rdstr(gd_t gd, char *buf, int len);
int gpib_rdf(gd_t gd, char *fmt, ...);

void gpib_wrt(gd_t gd, void *buf, int len);
void gpib_wrtstr(gd_t gd, char *str);
int gpib_wrtf(gd_t gd, char *fmt, ...);

void gpib_loc(gd_t gd);
void gpib_clr(gd_t gd, unsigned long usec);
void gpib_trg(gd_t gd);
int gpib_rsp(gd_t gd, unsigned char *status);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
