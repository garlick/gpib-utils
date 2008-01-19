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

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
