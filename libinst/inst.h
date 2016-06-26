#ifndef _INST_INST_H
#define _INST_INST_H 1

struct instrument;

/* Callback function to interpret results of serial poll.
 * Return value: -1=!ready, 0=success, >0=fatal error
 */
typedef int (*spollfun_t)(struct instrument *gd,
			  unsigned char status_byte, char *msg);

/* Initialize/finalize a device.  If sf is non-NULL, a serial poll is
 * run after every I/O and the resulting status byte is passed to the sf
 * function for processing.  In case serial poll returns not ready, 'retry'
 * is a backoff factor (in usecs) to sleep before retrying.
 */
struct instrument *inst_init(const char *addr, spollfun_t sf, unsigned long retry);
void inst_fini(struct instrument *gd);

/* Set the gpib timeout in seconds.
 */
void inst_set_timeout(struct instrument *gd, double sec);

/* Set flag that determines whether "telemetry" is displayed on
 * stderr as reads/writes are processed on the gpib.
 */
void inst_set_verbose(struct instrument *gd, int flag);

/* Set flag that determines whether EOI is set on the last byte
 * of every write.
 */
void inst_set_eot(struct instrument *gd, int flag);

/* Set flag that determines whether reads are terminated when
 * end-of-string is read.
 */
void inst_set_reos(struct instrument *gd, int flag);

/* Set character to be used as end-of-string terminator.
 */
void inst_set_eos(struct instrument *gd, int c);

int inst_rd(struct instrument *gd, void *buf, int len);
void inst_rdstr(struct instrument *gd, char *buf, int len);
int inst_rdf(struct instrument *gd, char *fmt, ...);

void inst_wrt(struct instrument *gd, void *buf, int len);
void inst_wrtstr(struct instrument *gd, char *str);
void inst_wrtf(struct instrument *gd, char *fmt, ...);

int inst_qry(struct instrument *gd, char *str, void *buf, int len);
int inst_qryint(struct instrument *gd, char *str);
int inst_qrystr(struct instrument *gd, char *str, char *buf, int len);

void inst_loc(struct instrument *gd);
void inst_clr(struct instrument *gd, unsigned long usec);
void inst_trg(struct instrument *gd);
int inst_rsp(struct instrument *gd, unsigned char *status);

void inst_abort(struct instrument *gd); /* VXI only */

#endif /* !INST_INST_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
