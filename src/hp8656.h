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


/* This instrument is listen-only (no SRQ, no serial/parallel poll).
 */

#define HP8656_FREQ        "FR"    /* frequency (carrier) */

#define HP8656_AMPL        "AP"    /* amplitude (carrier) */

#define HP8656_MOD_AM      "AM"    /* modulation - amplitude modulation */
#define HP8656_MOD_AF      "FM"    /* modulation - frequency modulation */
#define HP8656_MOD_S1      "S1"    /* modulation - external modulation source */
#define HP8656_MOD_S2      "S2"    /* modulation - internal 400 Hz mod source */
#define HP8656_MOD_S3      "S3"    /* modulation - internal 1 kHz mod source */
#define HP8656_MOD_S4      "S4"    /* modulation - source off */

#define HP8656_UNIT_DB     "DB"
#define HP8656_UNIT_DBF    "DF"
#define HP8656_UNIT_DBM    "DM"    /* 1 dBm = 0.22V rms at 50 ohm impedance */
#define HP8656_UNIT_EMF    "EM"
#define HP8656_UNIT_V      "VL"
#define HP8656_UNIT_MV     "MV"
#define HP8656_UNIT_UV     "UV"
#define HP8656_UNIT_HZ     "HZ"
#define HP8656_UNIT_KHZ    "KZ"
#define HP8656_UNIT_MHZ    "MZ"
#define HP8656_UNIT_PCT    "%"

#define HP8656_STEP_UP     "UP"
#define HP8656_STEP_DN     "DN"
#define HP8656_INCR_SET    "IS"
#define HP8656_STBY        "R0"
#define HP8656_ON          "R1"
#define HP8656_STORE       "ST"
#define HP8656_RECALL      "RC"
#define HP8656_SEQUENCE    "SQ"
#define HP8656_RPP_RST     "RP"    /* reverse power protection reset */


#define HP8656_FUN_AM      "AM"    /* amplitude modulation */
#define HP8656_FUN_AP      "AP"    /* amplitude (carrier) */
#define HP8656_FUN_DN      "DN"    /* step down */
#define HP8656_FUN_DN      "DN"    /* step down */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

