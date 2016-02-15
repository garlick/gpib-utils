#ifndef INST_488_2_H
#define INST_488_2_H 1

#include "libutil/util.h" /* for strtab_t */

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

#endif /* !INST_488_2_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
