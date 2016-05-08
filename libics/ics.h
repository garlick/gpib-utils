#ifndef _ICS_H
#define _ICS_H

/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2007 Jim Garlick <garlick.jim@gmail.com>

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

/* All functions return 0 on success, <0 on RPC error, >0 on ICS error */

enum {
    ICS_ERROR_CREATE = -2,
    ICS_ERROR_CLNT = -1,
    ICS_SUCCESS = 0,
    ICS_ERROR_SYNTAX = 1,
    ICS_ERROR_PARAMETER = 5,
    ICS_ERROR_UNSUPPORTED = 8,
};

typedef struct ics_struct *ics_t;

/* Get/set the current VXI-11 logical interface name.
 * Caller must free the string returned by the get function.
 */
int     ics_get_interface_name(ics_t ics, char **strp);
int     ics_set_interface_name(ics_t ics, char *str);

/* Get/set TCP timeout value.  An inactive TCP channel will be left
 * open this length of time before being closed.  A value of zero
 * means no timeout checking.
 */
int     ics_get_comm_timeout(ics_t ics, unsigned int *timeoutp);
int     ics_set_comm_timeout(ics_t ics, unsigned int timeout);

/* Get/set static IP mode.
 */
int     ics_get_static_ip_mode(ics_t ics, int *flagp);
int     ics_set_static_ip_mode(ics_t ics, int flag);

/* Get/set network IP number.
 */
int     ics_get_ip_number(ics_t ics, char **ipstrp);
int     ics_set_ip_number(ics_t ics, char *ipstr);

/* Get/set network mask.
 */
int     ics_get_netmask(ics_t ics, char **ipstrp);
int     ics_set_netmask(ics_t ics, char *ipstr);

/* Get/set network gateway
 */
int     ics_get_gateway(ics_t ics, char **ipstrp);
int     ics_set_gateway(ics_t ics, char *ipstr);

/* Get/set keepalive time (in seconds)
 */
int     ics_get_keepalive(ics_t ics, unsigned int *timep);
int     ics_set_keepalive(ics_t ics, unsigned int time);

/* Get/set gpib address.
 */
int     ics_get_gpib_address(ics_t ics, unsigned int *addrp);
int     ics_set_gpib_address(ics_t ics, unsigned int addr);

/* Get/set system controller mode.
 */
int     ics_get_system_controller(ics_t ics, int *flagp);
int     ics_set_system_controller(ics_t ics, int flag);

/* Get/set REN active at boot.
 */
int     ics_get_ren_mode(ics_t ics, int *flagp);
int     ics_set_ren_mode(ics_t ics, int flag);

/* Reload config from flash.
 */
int     ics_reload_config(ics_t ics);

/* Reload flash with factory defaults.
* */
int     ics_reload_factory(ics_t ics);

/* Commit (write) current config.
 */
int     ics_commit_config(ics_t ics);

/* Reboot the ics device.
 */
int     ics_reboot(ics_t ics);

/* Obtain the device identity string, which contains the FW revision, 
 * ICS product model number, etc. Caller must free the identity string.
 */
int     ics_idn_string(ics_t ics, char **strp);

/* Get the current contents of the error log.  Put a copy of the array
 * in 'errp' and its length in 'countp'.  Caller must free the error log.
 */
int     ics_error_logger(ics_t ics, unsigned int **errp, int *countp);

/* Initialize/finalize the ics module.
 */
int     ics_init (char *host, ics_t *icsp);
void    ics_fini(ics_t ics);

/* Decode an error value to a descriptive string.
 */
char *  ics_strerror(ics_t ics, int err);

/* Mapping of error numbers (from the error log) to descriptive text.
 */
#define ICS_ERRLOG { \
    { 1,    "VXI-11 syntax error" }, \
    { 3,    "GPIB device not accessible" }, \
    { 4,    "invalid VXI-11 link ID" }, \
    { 5,    "VXI-11 parameter error" }, \
    { 6,    "invalid VXI-11 channel, channel not established" }, \
    { 8,    "invalid VXI-11 operation" }, \
    { 9,    "insufficient resources" }, \
    { 11,   "device locked by another link ID" }, \
    { 12,   "device not locked" }, \
    { 15,   "I/O timeout error" }, \
    { 17,   "I/O error" }, \
    { 21,   "invalid GPIB address specified" }, \
    { 23,   "operation aborted (indicator, not a true error)" }, \
    { 29,   "channel already established" }, \
    { 60,   "channel not active" }, \
    {110,   "device already locked" }, \
    {111,   "timeout attempting to gain a device lock" }, \
    {999,   "unspecified/unknown error" }, \
    {1000,  "VXI-11 protocol error" }, \
    {2000,  "RPC protocol error" }, \
    {2001,  "unsupported RPC function" }, \
    {2002,  "insufficient RPC message length" }, \
    {0,     NULL}, \
}

#endif /* _ICS_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
