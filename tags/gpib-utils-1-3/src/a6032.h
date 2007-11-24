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

/* Status byte values
 */
#define A6032_STAT_OPER	128	/* an enabled condition in OPER has occurred */
#define A6032_STAT_RQS  32	/* device is requesting service */
#define A6032_STAT_ESB  16	/* an enabled condition in ESR has occurred */
#define A6032_STAT_MAV	8	/* message(s) available in the output queue */
#define A6032_STAT_MSG	4	/* advisory has been displayed on the scope */
#define A6032_STAT_USR	2	/* an enabled user event has occurred */
#define A6032_STAT_TRG	1	/* trigger has occurred */

/* Event status register values
 */
#define A6032_ESR_PON	128	/* power on */
#define A6032_ESR_URQ	64	/* user request */
#define A6032_ESR_CME	32	/* command error */
#define A6032_ESR_EXE	16	/* execution error */
#define A6032_ESR_DDE	8	/* device dependent error */
#define A6032_ESR_QYE	4	/* query error */
#define A6032_ESR_RQL	2	/* request control */
#define A6032_ESR_OPC	1	/* operation complete */


/* SCPI commands for this instrument are mnemonic (and plentiful!) enough 
 * that we do not need to go nuts defining them here.
 */
