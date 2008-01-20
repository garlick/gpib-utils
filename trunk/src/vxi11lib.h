gd_t vxi11_device_open(char *addr, spollfun_t sf, unsigned long retry);
void vxi11_device_close(gd_t gd);

int vxi11_device_read(gd_t gd, char *buf, int len);
void vxi11_device_write(gd_t gd, char *buf, int len);
void vxi11_device_local(gd_t gd);
void vxi11_device_clear(gd_t gd, unsigned long usec);
void vxi11_device_trigger(gd_t gd);
int vxi11_device_readstb(gd_t gd, unsigned char *status);
void vxi11_device_abort(gd_t gd);
void vxi11_device_docmd(gd_t gd, vxi11_docmd_cmd_t cmd, ...);

typedef enum { 
    VXI11_DOCMD_SEND_COMMAND = 0x020000,
    VXI11_DOCMD_BUS_STATUS   = 0x020001,
    VXI11_DOCMD_ATN_CONTROL  = 0x020002,
    VXI11_DOCMD_REN_CONTROL  = 0x020003,
    VXI11_DOCMD_PASS_CONTROL  = 0x020004,
    VXI11_DOCMD_BUS_ADDRESS   = 0x02000A,
    VXI11_DOCMD_IFC_CONTROL   = 0x020010,
} vxi11_docmd_cmd_t;

#define VXI_MAX_READWRITE   (0xFFFFFFFF - 1)

/* VXI-11 device_read 'reason' bits */
#define VXI_REASON_REQCNT   0x01    /* requested size transferred */
#define VXI_REASON_CHR      0x02    /* VXI_TERMCHRSET & termchar read */
#define VXI_REASON_END      0x04    /* end indicator read */

/* VXI-11 flags */
#define VXI_WAITLOCK        0x01
#define VXI_ENDW            0x08    /* only valid for device_write */
#define VXI_TERMCHRSET      0x80    /* only valid for device_read */

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

/* Internal errors */
#define VXI11_ERR_RPCERR    100
#define VXI11_ERR_ABRTINFO  101
#define VXI11_ERR_TOOBIG    102

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
    { VXI11_ERR_RPCERR,     "RPC error" }, \
    { VXI11_ERR_ABRTINFO,   "getaddrinfo on abort port failed" }, \
    { VXI11_ERR_TOOBIG,     "write buffer exceeds receive size" }, \
    { 0, 0 }, \
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
