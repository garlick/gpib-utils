#ifndef _INST_CMDLINE_H
#define _INST_CMDLINE H 1

#include <getopt.h>


/* Look up a device's default address in /etc/gpib-utils.conf
 * Result is allocated in static storage that may be overwritten on next call.
 */
char *gpib_default_addr(char *name);

typedef struct {
    char *sopt;
    char *lopt;
    char *desc;
} opt_desc_t;

void usage(opt_desc_t *tab);

#define GPIB_UTILS_CONF { \
    "$GPIB_UTILS_CONF", \
    "~/.gpib-utils.conf", \
    "/etc/gpib-utils.conf", \
   NULL, \
}

#define OPTIONS_COMMON "a:n:v"
#define OPTIONS_COMMON_LONG \
    {"address",         required_argument, 0, 'a'}, \
    {"name",            no_argument,       0, 'n'}, \
    {"verbose",         no_argument,       0, 'v'}

#define OPTIONS_COMMON_DESC \
    {"a", "address",   "set instrument address" }, \
    {"n", "name",      "set instrument name" }, \
    {"v", "verbose",   "show protocol on stderr" }

#define OPTIONS_COMMON_SWITCH \
    case 'a': \
    case 'n': \
    case 'v':

gd_t gpib_init_args(int argc, char *argv[], const char *opts, 
                    struct option *longopts, char *name, 
                    spollfun_t sf, unsigned long retry, int *opt_error);

#endif /* !INST_CMDLINE_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
