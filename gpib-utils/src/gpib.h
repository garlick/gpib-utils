typedef void (*srqfun_t)(int d);

int gpib_init(char *progname, char *instrument, int vflag);

int gpib_ibrd(int d, void *buf, int len);
void gpib_ibrdstr(int d, char *buf, int len);

void gpib_ibwrt(int d, void *buf, int len);
void gpib_ibwrtstr(int d, char *str);
void gpib_ibwrtf(int d, char *fmt, ...);

void gpib_ibloc(int d);
void gpib_ibclr(int d);
void gpib_ibtrg(int d);
void gpib_ibrsp(int d, char *status);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
