#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <assert.h>

#include "ics8000.h"
#include "ics.h"
#include "util.h"
#include "errstr.h"

extern char *prog;

#define ICS_MAGIC 0x23434666
struct ics_struct {
    int         ics_magic;
    CLIENT     *ics_clnt;
};

static errstr_t errtab[] = {
    { 0,    "success" },
    { 1,    "syntax error" },
    { 5,    "parameter error" },
    { 0,    NULL },
};

int
ics_errorlogger(ics_t ics, unsigned int *errs, int len, int *count)
{
    Error_Log_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = errorlogger_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error)
        fprintf(stderr, "%s: errorlogger: %s\n", prog, 
                finderr(errtab, r->error));
    else
        for (*count = 0; *count < r->count && *count < len; (*count)++)
            errs[*count] = r->errors[*count];
    return r->error;
}

int
ics_reboot(ics_t ics)
{
    Reboot_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = reboot_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error)
        fprintf(stderr, "%s: reboot: %s\n", prog, finderr(errtab, r->error));
    return r->error;
}

int
ics_idnreply(ics_t ics, char *str, int len)
{
    Idn_Resp *r;

    assert(ics->ics_magic == ICS_MAGIC);
    r = idnreply_1(NULL, ics->ics_clnt);
    if (r == NULL) {
        clnt_perror(ics->ics_clnt, prog);
        exit(1);
    }
    if (r->error)
        fprintf(stderr, "%s: idnreply: %s\n", prog, finderr(errtab, r->error));
    else
        memcpy(str, r->reply, len > r->length ? r->length : len);
    return r->error;
}


void
ics_fini(ics_t ics)
{
    assert(ics->ics_magic == ICS_MAGIC);
    if (ics->ics_clnt)
        clnt_destroy(ics->ics_clnt);
    ics->ics_magic = 0;
    free(ics);
}

ics_t
ics_init (char *host)
{
    ics_t new;

    new = xmalloc(sizeof(struct ics_struct));
    new->ics_magic = ICS_MAGIC;

    new->ics_clnt = clnt_create(host, ICSCONFIG, ICSCONFIG_VERSION, "tcp");
    if (new->ics_clnt == NULL) {
        clnt_pcreateerror(prog);
        ics_fini(new);
        new = NULL;
    }

    return new;
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

