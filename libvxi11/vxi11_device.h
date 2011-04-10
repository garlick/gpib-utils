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

vxi11dev_t vxi11_create(void);

void vxi11_destroy(vxi11dev_t v);

int vxi11_open(vxi11dev_t v, char *name, bool doAbort);

void vxi11_close(vxi11dev_t v);

int vxi11_write(vxi11dev_t v, char *buf, int len);

int vxi11_writestr(vxi11dev_t v, char *str);

int vxi11_read(vxi11dev_t v, char *buf, int len, int *numreadp);

int vxi11_readstr(vxi11dev_t v, char *str, int len);

int vxi11_readstb(vxi11dev_t v, unsigned char *stbp);

int vxi11_trigger(vxi11dev_t v);

int vxi11_clear(vxi11dev_t v);

int vxi11_remote(vxi11dev_t v);

int vxi11_local(vxi11dev_t v);

int vxi11_lock(vxi11dev_t v);

int vxi11_unlock(vxi11dev_t v);

int vxi11_abort(vxi11dev_t v);

void vxi11_set_iotimeout(vxi11dev_t v, unsigned long timeout);

void vxi11_set_lockpolicy(vxi11dev_t v, int useLocking, unsigned long timeout);

/* Sets the EOS character */
void vxi11_set_termchar(vxi11dev_t v, int termChar);

/* Sets whether reads are automatically terminated by EOS */
void vxi11_set_termcharset(vxi11dev_t v, int termCharSet);

/* Sets whether last char of write will assert EOI */
void vxi11_set_endw(vxi11dev_t v, bool doEndw);

void vxi11_set_device_debug(bool doDebug);

void vxi11_perror(vxi11dev_t v, int err, char *str);

#ifdef __cplusplus
};
#endif

#endif /* _VXI11_DEVICE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
