/* device handles */
typedef struct vxi11_handle_struct *vxi11_handle_t; 
vxi11_handle_t vxi11_open(char *host, char *device, 
                          int lockdevice, unsigned long locktimeout, 
                          unsigned long rpctimeout, int doabort, long *errp);
void vxi11_close(vxi11_handle_t vp);

/* vxi11 core/abort commands */
int vxi11_device_read(vxi11_handle_t vp, char *buf, int len, 
                      int *reasonp, long *errp);
int vxi11_device_readall(vxi11_handle_t vp, char *buf, int len, long *errp);
int vxi11_device_write(vxi11_handle_t vp, char *buf, int len, long *errp);
int vxi11_device_writeall(vxi11_handle_t vp, char *buf, int len, long *errp);
int vxi11_device_readstb(vxi11_handle_t vp, unsigned char *statusp, long *errp);
int vxi11_device_trigger(vxi11_handle_t vp, long *errp);
int vxi11_device_clear(vxi11_handle_t vp, long *errp);
int vxi11_device_remote(vxi11_handle_t vp, long *errp);
int vxi11_device_local(vxi11_handle_t vp, long *errp);
int vxi11_device_lock(vxi11_handle_t vp, long *errp);
int vxi11_device_unlock(vxi11_handle_t vp, long *errp);

int vxi11_device_docmd_sendcmds(vxi11_handle_t vp, char *cmds, 
                                int len, long *errp);
int vxi11_device_docmd_stat_ren(vxi11_handle_t vp, int *renp, long *errp);
int vxi11_device_docmd_stat_srq(vxi11_handle_t vp, int *srqp, long *errp);
int vxi11_device_docmd_stat_ndac(vxi11_handle_t vp, int *ndacp, long *errp);
int vxi11_device_docmd_stat_sysctl(vxi11_handle_t vp, int *sysctlp, long *errp);
int vxi11_device_docmd_stat_inchg(vxi11_handle_t vp, int *inchgp, long *errp);
int vxi11_device_docmd_stat_talk(vxi11_handle_t vp, int *talkp, long *errp);
int vxi11_device_docmd_stat_listen(vxi11_handle_t vp, int *listenp, long *errp);
int vxi11_device_docmd_stat_addr(vxi11_handle_t vp, int *addrp, long *errp);
int vxi11_device_docmd_atn(vxi11_handle_t vp, int atn, long *errp);
int vxi11_device_docmd_ren(vxi11_handle_t vp, int ren, long *errp);
int vxi11_device_docmd_pass(vxi11_handle_t vp, int addr, long *errp);
int vxi11_device_docmd_addr(vxi11_handle_t vp, int addr, long *errp);
int vxi11_device_docmd_ifc(vxi11_handle_t vp, long *errp);
int vxi11_device_abort(vxi11_handle_t vp, long *errp);

/* todo: intr channel calls */

/* accessors */
char         *vxi11_strerror(long err);
void          vxi11_set_waitlock(vxi11_handle_t vp, int waitlockflag);
int           vxi11_get_waitlock(vxi11_handle_t vp);
void          vxi11_set_eos(vxi11_handle_t vp, unsigned char tc);
unsigned char vxi11_get_eos(vxi11_handle_t vp);
void          vxi11_set_reos(vxi11_handle_t vp, int reos);
int           vxi11_get_reos(vxi11_handle_t vp);
void          vxi11_set_eot(vxi11_handle_t vp, int eot);
int           vxi11_get_eot(vxi11_handle_t vp);
void          vxi11_set_waitlock(vxi11_handle_t vp, int waitlock);
int           vxi11_get_waitlock(vxi11_handle_t vp);
void          vxi11_set_locktimeout(vxi11_handle_t vp, unsigned long msec);
unsigned long vxi11_get_locktimeout(vxi11_handle_t vp);
void          vxi11_set_iotimeout(vxi11_handle_t vp, unsigned long msec);
unsigned long vxi11_get_iotimeout(vxi11_handle_t vp);
void          vxi11_set_rpctimeout(vxi11_handle_t vp, unsigned long msec);
unsigned long vxi11_get_rpctimeout(vxi11_handle_t vp);
unsigned long vxi11_get_maxrecvsize(vxi11_handle_t vp);

/* VXI-11 device_read 'reason' bits */
#define VXI11_REASON_REQCNT 1   /* requested number of bytes read */
#define VXI11_REASON_CHR    2   /* read terminated by eos character */
#define VXI11_REASON_END    4   /* read terminated by EOI */

/* VXI-11 errors (specification 1.0) */
#define VXI11_ERR_SUCCESS   0
#define VXI11_ERR_SYNTAX    1
#define VXI11_ERR_NODEVICE  3
#define VXI11_ERR_LINKINVAL 4
#define VXI11_ERR_PARAMETER 5
#define VXI11_ERR_NOCHAN    6
#define VXI11_ERR_NOTSUPP   8
#define VXI11_ERR_RESOURCES 9
#define VXI11_ERR_LOCKED    11
#define VXI11_ERR_NOLOCK    12
#define VXI11_ERR_IOTIMEOUT 15
#define VXI11_ERR_IOERROR   17
#define VXI11_ERR_ADDRINVAL 21
#define VXI11_ERR_ABORT     23
#define VXI11_ERR_CHANEST   29
/* Errors added for this library */
#define VXI11_ERR_ABRTINFO  100
#define VXI11_ERR_TOOBIG    101
#define VXI11_ERR_BADRESP   102
/* Errors with this bit set belong to sunrpc */
#define VXI11_ERR_RPCERR    0x10000000

#define VXI11_ERRORS { \
    { VXI11_ERR_SUCCESS,    "no error" }, \
    { VXI11_ERR_SYNTAX,     "syntax error" }, \
    { VXI11_ERR_NODEVICE,   "device not accessible" }, \
    { VXI11_ERR_LINKINVAL,  "invalid link identifier" }, \
    { VXI11_ERR_PARAMETER,  "parameter error" }, \
    { VXI11_ERR_NOCHAN,     "channel not established" }, \
    { VXI11_ERR_NOTSUPP,    "operation not supported" }, \
    { VXI11_ERR_RESOURCES,  "out of resources" }, \
    { VXI11_ERR_LOCKED,     "device locked by another link" }, \
    { VXI11_ERR_NOLOCK,     "no lock held by this link" }, \
    { VXI11_ERR_IOTIMEOUT,  "I/O timeout" }, \
    { VXI11_ERR_IOERROR,    "I/O error" }, \
    { VXI11_ERR_ADDRINVAL,  "invalid address" }, \
    { VXI11_ERR_ABORT,      "abort" }, \
    { VXI11_ERR_CHANEST,    "channel already established" }, \
    { VXI11_ERR_ABRTINFO,   "getaddrinfo on abort port failed" }, \
    { VXI11_ERR_TOOBIG,     "write buffer exceeds max receive size" }, \
    { VXI11_ERR_BADRESP,    "unexpected response to query" }, \
    { 0, 0 }, \
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
