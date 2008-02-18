typedef struct vxi11_device_struct *vxi11dev_t;

vxi11dev_t vxi11_create(void);

void vxi11_destroy(vxi11dev_t v);

int vxi11_open(vxi11dev_t v, char *name, int doAbort);

void vxi11_close(vxi11dev_t v);

int vxi11_write(vxi11dev_t v, char *buf, int len);

int vxi11_writestr(vxi11dev_t v, char *str);

int vxi11_read(vxi11dev_t v, char *buf, int len, int *numreadp);

int vxi11_readstr(vxi11dev_t v, char *str, int len);

int vxi11_readstb(vxi11dev_t v, unsigned char *stbp);

int vxi11_trigger(vxi11dev_t v);

int vxi11_clear(vxi11dev_t v);

int vxi11_remote(vxi11dev_t v);

int vxi11_local(vxi11dev_t v);

int vxi11_lock(vxi11dev_t v);

int vxi11_unlock(vxi11dev_t v);

int vxi11_abort(vxi11dev_t v);

void vxi11_set_iotimeout(vxi11dev_t v, unsigned long timeout);

void vxi11_set_lockpolicy(vxi11dev_t v, int useLocking, unsigned long timeout);

void vxi11_set_termchar(vxi11dev_t v, int termChar);

void vxi11_set_endw(vxi11dev_t v, int doEndw);

void vxi11_perror(vxi11dev_t v, int err, char *str);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
