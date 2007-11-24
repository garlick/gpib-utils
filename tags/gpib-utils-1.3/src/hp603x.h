#define HP603X_VSET         "VSET"  /* set voltage setting: (xV or xMV) */
#define HP603X_VSET_QUERY   "VSET?" /* query voltage setting */
#define HP603X_VOUT_QUERY   "VOUT?" /* query measured voltage */

#define HP603X_ISET         "ISET"  /* set current setting (xA or xMA) */
#define HP603X_ISET_QUERY   "ISET?" /* query current setting */
#define HP603X_IOUT_QUERY   "IOUT?" /* query measured current */

/* Soft limits affect the maximum current or voltage accepted by vset/iset.
 */
#define HP603X_IMAX         "IMAX"  /* set soft current limit (xA or xMA) */
#define HP603X_IMAX_QUERY   "IMAX?" /* query soft current limit */
#define HP603X_VMAX         "VMAX"  /* set soft voltage limit (xV or xMV) */
#define HP603X_VMAX_QUERY   "VMAX?" /* query soft voltage limit */

#define HP603X_OVP          "OVP"   /* set OVP trip voltage */
#define HP603X_OVP_QUERY    "OVP?"  /* query OVP trip voltage */

/* Delay after programming a new output value before fault/srq can occur.
 */
#define HP603X_DLY          "DLY"   /* set delay (xS or xMS) - dflt 0.5s */
#define HP603X_DLY_QUERY    "DLY?"  /* query delay */

#define HP603X_OUT          "OUT"   /* enable/disable output (0 or 1) */
#define HP603X_OUT_QUERY    "OUT?"  /* query output status */

/* Foldback: 0=off, 1=cv (trip if switch to cc), 2=cc (trip if switch to cv)
 * When tripped, foldback disables output.
 */
#define HP603X_FOLD         "FOLD"  /* set foldback mode (0-2) */
#define HP603X_FOLD_QUERY   "FOLD?" /* query foldback mode */

#define HP603X_RST          "RST"   /* reset after foldback, OVP, or RI */

/* Hold and trigger.
 */
#define HP603X_HOLD         "HOLD"  /* 0 or 1 */
#define HP603X_HOLD_QUERY   "HOLD?"
#define HP603X_TRG          "TRG"

/* Store and recall.
 */
#define HP603X_STO          "STO"   /* 0-15 */
#define HP603X_RCL          "RCL"   /* 0-15 */

/* Fault = sts & mask.  Any bit set in fault reg sets HP603X_STATUS_FAU.
 */
#define HP603X_STS_QUERY    "STS?"  /* read 9-bit status reg (decimal)*/
#define HP603X_ASTS_QUERY   "ASTS?" /* accumulated version of same */
#define HP603X_UNMASK       "UNMASK"/* set mask reg value */
#define HP603X_UNMASK_QUERY "UNMASK?"/* query mask reg value */
#define HP603X_FAULT_QUERY  "FAULT?"/* query fault reg value */

#define HP603X_SRQ          "SRQ"   /* generate srq on STATUS_FAU (0-1) */

#define HP603X_CLR          "CLR"

/* Programming errors (decode with HP603X_ERRORS).
 * Sets HP603X_STATUS_ERR.
 */
#define HP603X_ERR_QUERY    "ERR?"  /* query error register content */

#define HP603X_TEST_QUERY   "TEST?" /* run tests, return result (0=pass) */

#define HP603X_ID_QUERY     "ID?"   /* query unit model number */

/* status, accumulated status, mask, and fault register bit values */
#define HP603X_STS_CV       1       /* constant voltage mode */
#define HP603X_STS_CC       2       /* constant current mode */
#define HP603X_STS_OR       4       /* overrange */
#define HP603X_STS_OV       8       /* over-voltage protection tripped */
#define HP603X_STS_OT       16      /* over-temp protection tripped */ 
#define HP603X_STS_AC       32      /* AC line dropout or out of rnage */
#define HP603X_STS_FOLD     64      /* foldback protection tripped */
#define HP603X_STS_ERR      128     /* remote programming error */
#define HP603X_STS_RI       256     /* remote inhibit */

/* serial poll status byte bit defs */
#define HP603X_STATUS_FAU   0x01    /* a bit is set in fault reg */
#define HP603X_STATUS_PON   0x02
#define HP603X_STATUS_RDY   0x10
#define HP603X_STATUS_ERR   0x20
#define HP603X_STATUS_RQS   0x40

#define HP603X_ERRORS { \
    { 0, "success" }, \
    { 1, "unrecognized character" }, \
    { 2, "improper number" }, \
    { 3, "unrecognized string" }, \
    { 4, "syntax error" }, \
    { 5, "number out of range" }, \
    { 6, "attempt to exceed soft limits" }, \
    { 7, "improper soft limit" }, \
    { 8, "data requested without a query being sent" },        \
    { 9, "relay error" }, \
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

