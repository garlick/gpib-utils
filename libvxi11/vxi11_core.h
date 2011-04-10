/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.
  
   Copyright (C) 2001-2010 Jim Garlick <garlick.jim@gmail.com>
  
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

/* All functions return: 0 on success, <0 on RPC error, >0 on VXI-11 error */
#define VXI11_ABRT_CREATE   (-4)
#define VXI11_CORE_CREATE   (-3)
#define VXI11_ABRT_RPCERR   (-2)
#define VXI11_CORE_RPCERR   (-1)

/* Open core channel.  Host may be an IP address or hostname in text form.
 * You will probably want to call vxi11_create_link() next.
 * Be sure to close with vxi11_close_core_channel().
 */
int vxi11_open_core_channel(char *host, CLIENT **corep);

/* Close core channel opened with vxi11_open_core_channel().
 */
void vxi11_close_core_channel(CLIENT *core);

/* Open abort channel - use port returned from vxi11_create_link().
 * This enables vxi11_device_abort() on the link.  The abort channel is
 * optional. Be sure to close with vxi11_close_abrt_channel().
 */
int vxi11_open_abrt_channel(CLIENT *core, unsigned short abortPort, 
                            CLIENT **abrtp);

/* Close abort channel opened with vxi11_open_abrt_channel().
 * It is best to close the abort channel before vxi11_destroy_link().
 */
void vxi11_close_abrt_channel(CLIENT *abrt);

/* Establish an instrument link.  The 'clientId' parameter is generally
 * not used (set to zero).  If you wish to block until this command can
 * run with exclusive access to the instrument, set 'lockDevice' to true
 * (o/w set it to false).  This call will then block up to 'lock_timeout' 
 * milliseconds until the instrument is available.  The 'device' parameter 
 * selects the target of the instrument link.  It may be "instr0" for 
 * standalone VXI-11 instruments; for a VXI-11 GPIB-ethernet gateway, 
 * use the form "gpib0" for the interface, or "gpib0,n" where n is 
 * the GPIB address for an instrument.  The link is returned in 'lidp' -
 * this is passed with the core channel handle to most of the vxi11 functions.
 * The port to pass into vxi11_open_abrt_channel() is returned in 'abortPortp' 
 * if non-NULL.  Finally, the maximum buffer that the instrument can handle 
 * in one vxi11_device_write() call is returned in 'maxRecvSizep' if non-NULL.
 */
int vxi11_create_link(CLIENT *core, int clientId, bool lockDevice, 
                  unsigned long lock_timeout, char *device, long *lidp, 
                  unsigned short *abortPortp, unsigned long *maxRecvSizep);

/* Write to device identified by 'core' channel handle and lid.  
 * Wait up to 'io_timeout' milliseconds for the I/O to complete.  
 * If the VXI11_FLAG_WAITLOCK bit is set in 'flags', block up to 'lock_timeout'
 * milliseconds until exclusive access to the instrument is available.
 * If the VXI11_FLAG_ENDW bit is set in 'flags', signify to the instrument
 * that the write is complete when the last byte completes (like GPIB EOI).
 * 'data_val' should be set to the address of the data to be written, and
 * 'data_len' to its length.  The amount of data successfully written is 
 * returned in 'sizep' if non-NULL.  The VXI-11 specification dictates that 
 * devices accept at least 1024 bytes in one write call, but the actual 
 * maximum is returned in the vxi11_create_link() call.  Data is copied 
 * internally so the storage associated with 'data_val' may be freed 
 * immediately upon return.
 */
int vxi11_device_write(CLIENT *core, long lid, long flags, 
                   unsigned long io_timeout, unsigned long lock_timeout,
                   char *data_val, int data_len, unsigned long *sizep);

/* Read from device identified by 'core' channel handle and lid.
 * Wait up to 'io_timeout' milliseconds for the I/O to complete.  
 * If the VXI11_FLAG_WAITLOCK bit is set in 'flags', block up to 'lock_timeout'
 * milliseconds until exclusive access to the instrument is available.
 * If the VXI11_FLAG_TERMCHRSET bit is set in 'flags', terminate the read
 * as soon as 'termChar' appears in the data stream, and return it as the
 * last character.  Up to 'requestSize' bytes will be read.
 * If non-NULL, 'reasonp' will be set to indicate a mask of reasons the 
 * read returned: the VXI11_REASON_REQCNT bit will be set if the requested 
 * number of bytes were read; the VXI11_REASON_CHR bit will be set if the 
 * read was terminated by 'termChar' as described above; and the 
 * VXI11_REASON_END bit will be set if the read was terminated by the
 * end signal (like GPIB EOI).  If non-NULL, the the returned data
 * and its length are copied into 'data_lenp' and 'data_val' respectively.
 */
int vxi11_device_read(CLIENT *core, long lid, long flags,
                  unsigned long io_timeout, unsigned long lock_timeout, 
                  int termChar, int *reasonp, 
                  char *data_val, int *data_lenp, unsigned long requestSize);

/* Read status byte from device identified by 'core' channel handle and lid.
 * Wait up to 'io_timeout' milliseconds for the I/O to complete.  
 * If the VXI11_FLAG_WAITLOCK bit is set in 'flags', block up to 'lock_timeout'
 * milliseconds until exclusive access to the instrument is available.
 * If non-NULL the instrument status byte will be stored in 'stbp'
 */
int vxi11_device_readstb(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout,
                     unsigned char *stbp);

/* Trigger the device identified by 'core' channel handle and lid.
 * Wait up to 'io_timeout' milliseconds for the I/O to complete.  
 * If the VXI11_FLAG_WAITLOCK bit is set in 'flags', block up to 'lock_timeout'
 * milliseconds until exclusive access to the instrument is available.
 */
int vxi11_device_trigger(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout);

/* Clear the device identified by 'core' channel handle and lid.
 * Wait up to 'io_timeout' milliseconds for the I/O to complete.  
 * If the VXI11_FLAG_WAITLOCK bit is set in 'flags', block up to 'lock_timeout'
 * milliseconds until exclusive access to the instrument is available.
 */
int vxi11_device_clear(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout);

/* Disable front panel access to the device identified by 'core' channel handle
 * and lid.  Wait up to 'io_timeout' milliseconds for the I/O to complete.  
 * If the VXI11_FLAG_WAITLOCK bit is set in 'flags', block up to 'lock_timeout'
 * milliseconds until exclusive access to the instrument is available.
 */
int vxi11_device_remote(CLIENT *core, long lid, long flags,
                     unsigned long io_timeout, unsigned long lock_timeout);

/* Enable front panel access to the device identified by 'core' channel handle 
 * and lid.  Wait up to 'io_timeout' milliseconds for the I/O to complete.  
 * If the VXI11_FLAG_WAITLOCK bit is set in 'flags', block up to 'lock_timeout'
 * milliseconds until exclusive access to the instrument is available.
 */
int vxi11_device_local(CLIENT *core, long lid, long flags,
                   unsigned long io_timeout, unsigned long lock_timeout);

/* Lock the device identified by 'core' channel handle and lid.  
 * If the VXI11_FLAG_WAITLOCK bit is set in 'flags', block up to 'lock_timeout'
 * milliseconds until exclusive access to the instrument is available.
 */
int vxi11_device_lock(CLIENT *core, long lid, long flags, 
                  unsigned long lock_timeout);

/* Unlock the device identified by 'core' channel handle and lid.  
 */
int vxi11_device_unlock(CLIENT *core, long lid);

int vxi11_device_enable_srq(CLIENT *core, long lid, bool enable, 
                        char *handle_val, int handle_len);

int vxi11_device_docmd(CLIENT *core, long lid, long flags, 
                   unsigned long io_timeout, unsigned long lock_timeout,
                   long cmd, int network_order, long datasize, 
                   char *data_in_val, int data_in_len,
                   char **data_out_valp, int *data_out_lenp);

int vxi11_destroy_link(CLIENT *core, long lid);

int vxi11_create_intr_chan(CLIENT *core, unsigned long hostAddr,
                       unsigned short hostPort, unsigned long progNum,
                       unsigned long progVers, int progFamily);

int vxi11_destroy_intr_chan(CLIENT *core);

int vxi11_device_abort(CLIENT *abrt, long lid);

void vxi11_set_core_debug(bool doDebug);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
