#ifndef _VXI11_DEVICE_H
#define _VXI11_DEVICE_H

/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.
  
   Copyright (C) 2001-2011 Jim Garlick <garlick.jim@gmail.com>
  
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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vxi11_device_struct *vxi11dev_t;

/* Create a vxi11 device handle.
 * Returns object or NULL on out of memory error.
 */
vxi11dev_t vxi11_create(void);

/* Destroy a vxi11 device handle.
 * This function always succeeds.
 */
void vxi11_destroy(vxi11dev_t v);

/* Using a vxi11 device handle from vxi11_create (), open a the core
 * channel to the vxi11 instrument specified by 'name', constructed from
 * "hostname:device", e.g. "myscope:instr0" or "gateway:gpib0,15".
 * If 'doAbort' is true, also open the abort channel, which allows
 * vxi11_abort () to be called on the handle.
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_open(vxi11dev_t v, char *name, bool doAbort);

/* Close an vxi11 device handle previously opened with vxi11_open ().
 * If the handle has an open abort channel, close that too.
 * This function always succeeds.
 */
void vxi11_close(vxi11dev_t v);

/* Write 'len' bytes of 'buf' to the open vxi11 device handle.
 * If vxi11_set_endw () has been called, set the ENDW flag on the last
 * chunk written.
 * VXI Locking is employed if so configured - see vxi11_set_lockpolicy ().
 * After 30s or the value configured with vxi11_set_iotimeout (),
 * the operation will time out and return an error.
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_write(vxi11dev_t v, char *buf, int len);

/* Write 'str' (null terminated) to the open vxi11 device handle.
 * Otherwise it is the same as vxi11_write ().
 */
int vxi11_writestr(vxi11dev_t v, char *str);

/* Read at most 'len' bytes into 'buf' from the open vxi11 device handle.
 * The number of bytes read is returned in 'numreadp'.
 * If a termChar is configured the read is terminated when that character
 * is received - see vxi11_set_termcharset () and vxi11_set_termchar ().
 * VXI Locking is employed if so configured - see vxi11_set_lockpolicy ().
 * After 30s or the value configured with vxi11_set_iotimeout (),
 * the operation will time out and return an error.
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_read(vxi11dev_t v, char *buf, int len, int *numreadp);

/* Read at most 'len' - 1 bytes into 'buf' from the open vxi11 device handle,
 * adding a terminating NULL.
 */
int vxi11_readstr(vxi11dev_t v, char *str, int len);

/* Read the status byte into 'stbp' from an open vxi11 device handle.
 * VXI Locking is employed if so configured - see vxi11_set_lockpolicy ().
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_readstb(vxi11dev_t v, unsigned char *stbp);

/* Trigger instrument via an open vxi11 device handle.
 * VXI locking is employed if so configured - see vxi11_set_lockpolicy ().
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_trigger(vxi11dev_t v);

/* Clear instrument via an open vxi11 device handle.
 * VXI locking is employed if so configured - see vxi11_set_lockpolicy ().
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_clear(vxi11dev_t v);

/* Disable local control of instrument via an open vxi11 device handle.
 * VXI locking is employed if so configured - see vxi11_set_lockpolicy ().
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_remote(vxi11dev_t v);

/* Enable local control of instrument via an open vxi11 device handle.
 * VXI locking is employed if so configured - see vxi11_set_lockpolicy ().
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_local(vxi11dev_t v);

/* Take a (blocking) VXI11 lock on an open vxi11 device handle. 
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_lock(vxi11dev_t v);

/* Give up a VXI11 lock on an open vxi11 device handle.
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_unlock(vxi11dev_t v);

/* Abort any outstanding requests on an open vxi11 device handle.
 * Requires that device was opened with 'doAbort' true.
 * Returns 0 on success or an error code, decoded with vxi11_strerror ().
 */
int vxi11_abort(vxi11dev_t v);

/* Change the I/O timeout on a vxi11 device handle from the default of
 * 30s to 'timeout' milliseconds.
 * This function always succeeds.
 */
void vxi11_set_iotimeout(vxi11dev_t v, unsigned long timeout);

/* Change the lock policy on a vxi11 device handle.
 * By default, blocking VXI locks are not taken implicitly by the above
 * functions.  By setting 'useLocking' true, the functions will block if
 * there is an outstanding request.  The default lock timeout of 30s
 * can also be overridden by setting 'timeout' to a value in milliseconds.
 * This function always succeeds.
 */
void vxi11_set_lockpolicy(vxi11dev_t v, int useLocking, unsigned long timeout);

/* Changes the character which terminates reads from 0x0a to 'termChar'
 * for a vxi11 device handle.  The termChar is only used if
 * vxi11_set_termcharset () has enabled this functionality.
 * This function always succeeds.
 */
void vxi11_set_termchar(vxi11dev_t v, int termChar);

/* Enable/disable 'termCharSet', the termination of vxi11_read () when a
 * particular character is received, on a vxi11 device handle.
 * This function always succeeds.
 */
void vxi11_set_termcharset(vxi11dev_t v, bool termCharSet);

/* Enable/disable the behavior of setting the ENDW flag in the last
 * chunk of a vxi11_write () for a vxi11 device handle.
 * This function always succeeds.
 */
void vxi11_set_endw(vxi11dev_t v, bool doEndw);

/* Enable/disable debugging on stderr.
 */
void vxi11_set_device_debug(bool doDebug);

/* Convert an error code returned from one of the above functions to a string.
 * The string is statically allocated (per vxi11 handle) and overwritten on
 * the next vxi11_strerror () or vxi11_perror () call for that handle.
 * This function always succeeds.
 */ 
char *vxi11_strerror(vxi11dev_t v, int err);

/* Decode 'err' to an error string and print it using the follwoing format:
 *   fprintf(stderr, "%s (%s): %s\n", str, v->vxi11_devname, errstr);
 * This function always succeeds.
 */
void vxi11_perror(vxi11dev_t v, int err, char *str);
#ifdef __cplusplus
};
#endif

#endif /* _VXI11_DEVICE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
