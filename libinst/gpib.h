#ifndef _INST_GPIB_H
#define _INST_GPIB_H 1

typedef struct gpib_device *gd_t;

/* Callback function to interpret results of serial poll.
 * Return value: -1=!ready, 0=success, >0=fatal error
 */
typedef int (*spollfun_t)(gd_t gd, unsigned char status_byte, char *msg);

/* Initialize/finalize a device.  If sf is non-NULL, a serial poll is 
 * run after every I/O and the resulting status byte is passed to the sf
 * function for processing.  In case serial poll returns not ready, 'retry' 
 * is a backoff factor (in usecs) to sleep before retrying.
 */
gd_t gpib_init(char *addr, spollfun_t sf, unsigned long retry);
void gpib_fini(gd_t gd);

/* Set the gpib timeout in seconds.
 */
void gpib_set_timeout(gd_t gd, double sec);

/* Set flag that determines whether "telemetry" is displayed on
 * stderr as reads/writes are processed on the gpib.
 */
void gpib_set_verbose(gd_t gd, int flag);

/* Set flag that determines whether EOI is set on the last byte
 * of every write.
 */
void gpib_set_eot(gd_t gd, int flag);

/* Set flag that determines whether reads are terminated when
 * end-of-string is read.
 */
void gpib_set_reos(gd_t gd, int flag);

/* Set character to be used as end-of-string terminator.
 */
void gpib_set_eos(gd_t gd, int c);

int gpib_rd(gd_t gd, void *buf, int len);
void gpib_rdstr(gd_t gd, char *buf, int len);
int gpib_rdf(gd_t gd, char *fmt, ...);

void gpib_wrt(gd_t gd, void *buf, int len);
void gpib_wrtstr(gd_t gd, char *str);
void gpib_wrtf(gd_t gd, char *fmt, ...);

int gpib_qry(gd_t gd, char *str, void *buf, int len);
int gpib_qryint(gd_t gd, char *str);
int gpib_qrystr(gd_t gd, char *str, char *buf, int len);

void gpib_loc(gd_t gd);
void gpib_clr(gd_t gd, unsigned long usec);
void gpib_trg(gd_t gd);
int gpib_rsp(gd_t gd, unsigned char *status);

void gpib_abort(gd_t gd); /* VXI only */

#endif /* !INST_GPIB_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
