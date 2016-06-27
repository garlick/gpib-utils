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

struct cf_instrument {
    char name[MAX_GPIB_NAME];
    char addr[MAX_GPIB_ADDR];
    char make[MAX_GPIB_MAKE];
    char model[MAX_GPIB_MODEL];
    char idncmd[MAX_GPIB_IDNCMD];
    char eos;
    int flags;
};

struct cf_file;

void cf_list (struct cf_file *cf, FILE *f);
struct cf_file *cf_create_default (void);
struct cf_file *cf_create (const char *pathname);
void cf_destroy (struct cf_file *cf);

const struct cf_instrument *cf_lookup (struct cf_file *cf, const char *name);

#endif /* !GPIB_CONFIGFILE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
