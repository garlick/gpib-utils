/* This file is part of gpib-utils.
   For details, see http://github.com/garlick/gpib-utils

   Copyright (C) 2016 Jim Garlick <garlick.jim@gmail.com>

   gpib-utils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   gpib-utils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with gpib-utils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libini/ini.h"
#include "libutil/util.h"
#include "liblsd/hash.h"

#include "configfile.h"

#define CF_MAGIC 0xeef0aa32
struct cf_file {
    int magic;
    hash_t db;
};

static int parse_flags (const char *flags, int *fp)
{
    char *cpy = xstrdup (flags);
    char *flag, *saveptr = NULL, *a1 = cpy;
    while ((flag = strtok_r (a1, ",", &saveptr))) {
        if (!strcmp (flag, "reos")) {
            *fp |= GPIB_FLAG_REOS;
        } else {
            fprintf (stderr, "Unknown flag in config file '%s'\n", flag);
            return -1;
        }
        a1 = NULL;
    }
    return 0;
}

static void cfi_destroy (struct cf_instrument *cfi)
{
    if (cfi) {
        free (cfi);
    }
}

static struct cf_instrument *cfi_create (struct cf_file *cf, const char *name)
{
    int saved_errno;
    struct cf_instrument *cfi = xzmalloc (sizeof (*cfi));

    snprintf (cfi->name, MAX_GPIB_NAME, "%s", name);
    if (hash_insert (cf->db, cfi->name, cfi) == NULL)
        goto error;
    return cfi;
error:
    saved_errno = errno;
    cfi_destroy (cfi);
    errno = saved_errno;
    return NULL;
}


const struct cf_instrument *cf_lookup (struct cf_file *cf, const char *name)
{
    return hash_find (cf->db, name);
}

static int parse_cb (void *arg, const char *section, const char *name,
                     const char *value)
{
    struct cf_file *cf = arg;
    struct cf_instrument *cfi = hash_find (cf->db, section);

    if (!cfi)
        cfi = cfi_create (cf, section);
    if (!cfi)
        goto error;
    if (!strcmp (name, "address"))
        snprintf (cfi->addr, MAX_GPIB_ADDR, "%s", value);
    else if (!strcmp (name, "manufacturer"))
        snprintf (cfi->make, MAX_GPIB_MAKE, "%s", value);
    else if (!strcmp (name, "model"))
        snprintf (cfi->model, MAX_GPIB_MODEL, "%s", value);
    else if (!strcmp (name, "idn"))
        snprintf (cfi->idncmd, MAX_GPIB_IDNCMD, "%s", value);
    else if (!strcmp (name, "flags")) {
        if (parse_flags (value, &cfi->flags) < 0)
            goto error;
    } else {
        fprintf (stderr, "unknown config file attribute '%s'\n", name);
        goto error;
    }
    return 1; /* ini success */
error:
    return 0; /* ini error */
}

static int cfi_list (void *data, const void *key, void *arg)
{
    struct cf_instrument *cfi = data;
    FILE *f = arg;
    fprintf (f, "%-16s %-30s %s%s%s\n",
             cfi->name,
             cfi->addr,
             cfi->make ? cfi->make : "",
             cfi->make ? " " : "",
             cfi->model);
    return 1;
}

void cf_list (struct cf_file *cf, FILE *f)
{
    hash_for_each (cf->db, (hash_arg_f)cfi_list, f);
}

struct cf_file *cf_create (const char *pathname)
{
    struct cf_file *cf = xzmalloc (sizeof (*cf));
    int saved_errno;
    int rc;

    cf->magic = CF_MAGIC;
    cf->db = hash_create (0, (hash_key_f)hash_key_string,
                             (hash_cmp_f)strcmp,
                             (hash_del_f)cfi_destroy);
    rc = ini_parse (pathname, parse_cb, cf);
    if (rc == -1) { /* open error */
        goto error;
    } else if (rc == -2) { /* out of mem */
        errno = ENOMEM;
        goto error;
    } else if (rc > 0) { /* line number */
        fprintf (stderr, "%s line %d: parse error\n", pathname, rc);
        errno = EINVAL;
        goto error;
    }
    return cf;
error:
    saved_errno = errno;
    cf_destroy (cf);
    errno = saved_errno;
    return NULL;
}

void cf_destroy (struct cf_file *cf)
{
    if (cf) {
        if (cf->db)
            hash_destroy (cf->db);
        cf->magic = ~CF_MAGIC;
        free (cf);
    }
}

struct cf_file *cf_create_default (void)
{
    struct cf_file *cf = NULL;
    char path[PATH_MAX];
    char *env;

    if ((env = getenv ("GPIB_UTILS_CONF")))
        cf = cf_create (env);
    if (!cf) {
        struct passwd *pw = getpwuid (geteuid ());
        if (pw) {
            snprintf (path, sizeof (path), "%s/.gpib-utils.conf", pw->pw_dir);
            cf = cf_create (path);
        }
    }
    if (!cf) {
        snprintf (path, sizeof (path), "%s/gpib-utils.conf", X_SYSCONFDIR);
        cf = cf_create (path);
    }
    return cf;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
