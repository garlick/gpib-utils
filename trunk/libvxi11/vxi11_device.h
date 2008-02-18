typedef struct vxi11_device_struct *vxi11dev_t;

vxi11dev_t vxi11_create(char *name);

void vxi11_destroy(vxi11dev_t v);

int vxi11_open_core(vxi11dev_t v);

void vxi11_close_core(vxi11dev_t v);

int vxi11_open_abort(vxi11dev_t v);

void vxi11_close_abort(vxi11dev_t v);

int vxi11_write(vxi11dev_t v, char *buf, int len);

int vxi11_read(vxi11dev_t v, char *buf, int len, int *numreadp);

int vxi11_reads(vxi11dev_t v, char *str, int len);

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

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
