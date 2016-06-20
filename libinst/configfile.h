#ifndef _GPIB_CONFIGFILE_H
#define _GPIB_CONFIGFILE_H 1

#define MAX_GPIB_NAME 64
#define MAX_GPIB_ADDR 128
#define MAX_GPIB_MAKE 64
#define MAX_GPIB_MODEL 64
#define MAX_GPIB_IDNCMD 64

enum {
    GPIB_FLAG_REOS = 1,
};

struct gpib_instrument {
    char name[MAX_GPIB_NAME];
    char addr[MAX_GPIB_ADDR];
    char make[MAX_GPIB_MAKE];
    char model[MAX_GPIB_MODEL];
    char idncmd[MAX_GPIB_IDNCMD];
    char eos;
    int flags;
};

#endif /* !GPIB_CONFIGFILE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
