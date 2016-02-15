#ifndef INST_488_2_H
#define INST_488_2_H 1

#include "libutil/util.h" /* for strtab_t */
#include "inst.h"

/* 488.2 functions */

#define INST_DECODE_DLAB    1
#define INST_DECODE_ILAB    2
#define INST_VERIFY_DLAB    4
#define INST_VERIFY_ILAB    8
int inst_decode_488_2_data(unsigned char *data, int *lenp, int flags);

/* required status reporting common commands */
void inst_488_2_cls(struct instrument *gd);
int inst_488_2_ese(struct instrument *gd);
int inst_488_2_esr(struct instrument *gd);
int inst_488_2_sre(struct instrument *gd);
int inst_488_2_stb(struct instrument *gd);

/* required internal operation common commands */
void inst_488_2_idn(struct instrument *gd);
void inst_488_2_rst(struct instrument *gd, int delay_secs);
void inst_488_2_tst(struct instrument *gd, strtab_t *tab);

/* optional learn command (and restore helper function) */
void inst_488_2_lrn(struct instrument *gd);
void inst_488_2_restore(struct instrument *gd);

/* optional option identification command */
void inst_488_2_opt(struct instrument *gd);

#endif /* !INST_488_2_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
