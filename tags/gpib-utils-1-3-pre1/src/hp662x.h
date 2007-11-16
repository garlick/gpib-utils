#define HP662X_DSP_QUERY    "DSP?"  /* query display - 1(on) or 0(off) */
#define HP662X_DSP          "DSP"   /* DSP "string" */

#define HP662X_ERR_QUERY    "*ERR?" /* query error register */
#define HP662X_FLT_QUERY    "*FAULT?" /* *FAULT? <ch> - query chan fault reg */
#define HP662X_ID_QUERY     "ID?"

/* calibration */
#define HP662X_IDATA        "IDATA" /* IDATA <ch>,<Ilo>,<Ihi> */
#define HP662X_IHI          "IHI"   /* IHI <ch> go to high cal point */
#define HP662X_ILO          "ILO"   /* ILO <ch> go to low cal point */
#define HP662X_OVCAL        "OVCAL" /* OVCAL <ch> go thru OV cal routine */
#define HP662X_VHI          "VHI"   /* VHI <ch> set V to high cal pt */
#define HP662X_VLO          "VLO"   /* VLO <ch> set V to low cal pt */

#define HP662X_IOUT_QUERY   "IOUT?" /* IOUT? <ch> query meas. output current */
#define HP662X_ISET         "*ISET" /* *ISET <ch>,<current> set current */
#define HP662X_ISET_QUERY   "*ISET?"/* *ISET <ch> */
#define HP662X_OCP          "*OCP"  /* *OCP <ch>,<0|1> enable overcurrent prot*/
#define HP662X_OCP_QUERY    "*OCP?" /* *OCP <ch> */
#define HP662X_OCRST        "*OCRST"/* *OCRST <ch> return to previous */
                                    /*   setting after OCP shutoff */
#define HP662X_OUT          "*OUT"  /* *OUT <ch>,<0|1> enable output */
#define HP662X_DCPON        "DCPON" /* DCPON <0|1> set power-on state */
#define HP662X_OUT_QUERY    "OUT?"  /* OUT? <ch> */
#define HP662X_OVSET        "*OVSET"/* *OVSET <ch>,<voltage> set OV trip pt */
#define HP662X_OVRST        "*OVRST"/* *OVRST <ch> reset OV crowbar */
#define HP662X_OVSET_QUERY  "*OVSET?"/* *OVSET <ch> query OV setting (real) */
#define HP662X_PON          "PON"   /* PON <0|1> enable PON SRQ */
#define HP662X_PON_QUERY    "PON?"  /* query PON */
#define HP662X_RCL          "*RCL"  /* *RCL <reg> recall V and I from reg 1-10*/
#define HP662X_ROM_QUERY    "ROM?"  /* query f/w rev date */
#define HP662X_SRQ          "SRQ"   /* SRQ <0|1|2|3> */
#define HP662X_SRQ_QUERY    "SRQ?"
#define HP662X_STO          "*STO"  /* *STO <reg> store I and V to reg 1-10 */
#define HP662X_STS_QUERY    "STS?"  /* STS? <ch> query preset status */
#define HP662X_TEST_QUERY   "TEST?" /* execute self test */
#define HP662X_VDATA        "VDATA" /* VDATA <ch>,<Vlo>,<Vhi> cal V */
#define HP662X_VMUX_QUERY   "VMUX?" /* VMUX? <ch>,<input> */
#define HP662X_VOUT_QUERY   "VOUT?" /* VOUT? <ch> query meas output */
#define HP662X_VSET         "*VSET" /* *VSET <ch>,<voltage> */
#define HP662X_UNMASK       "*UNMASK"/* *UNMASK <ch>,<setting> */
#define HP662X_UNMASK_QUERY "*UNMASK?"/* *UNMASK? <ch> */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

