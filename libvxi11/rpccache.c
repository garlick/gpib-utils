/* rpccache.c - cache RPC connections for reuse */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <rpc/pmap_clnt.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/time.h>

#include "rpccache.h"

#define CLNT_CACHE_MAGIC 0x346abefa
struct clnt_cache_struct {
    int magic;
    char host[MAXHOSTNAMELEN];
    u_long prog;
    u_long vers;
    char proto[MAXHOSTNAMELEN];
    CLIENT *clnt;
    int usecount;
    struct clnt_cache_struct *next;
};
static struct clnt_cache_struct *clnt_cache = NULL;

CLIENT *
clnt_create_cached(char *host, u_long prog, u_long vers, char *proto)
{
    CLIENT *clnt;
    struct clnt_cache_struct *cp, *new;

    for (cp = clnt_cache; cp != NULL; cp = cp->next) {
        assert(cp->magic == CLNT_CACHE_MAGIC);
        if (!strcmp(cp->host, host) && !strcmp(cp->proto, proto) && 
                cp->prog == prog && cp->vers == vers) {
            cp->usecount++;
            return cp->clnt;
        }
    }
    if ((clnt = clnt_create(host, prog, vers, proto))) {
        if ((new = malloc(sizeof(struct clnt_cache_struct)))) {
            new->magic = CLNT_CACHE_MAGIC;
            strncpy(new->host, host, MAXHOSTNAMELEN);
            new->host[MAXHOSTNAMELEN - 1] = '\0';
            new->prog = prog;
            new->vers = vers;
            strncpy(new->proto, proto, MAXHOSTNAMELEN);
            new->proto[MAXHOSTNAMELEN - 1] = '\0';
            new->clnt = clnt;
            new->usecount = 1;
            new->next = clnt_cache;
            clnt_cache = new;
        }
    }
    return clnt;
}

void
clnt_destroy_cached(CLIENT *clnt)
{
    struct clnt_cache_struct *cp, *prev = NULL;

    for (cp = clnt_cache; cp != NULL; cp = cp->next) {
        assert(cp->magic == CLNT_CACHE_MAGIC);
        if (cp->clnt == clnt) {
                if (--cp->usecount == 0) {
                    clnt_destroy(clnt);
                    if (prev == NULL)
                        clnt_cache = cp->next;
                    else
                        prev->next = cp->next;
                    memset(cp, 0, sizeof(struct clnt_cache_struct));
                    free(cp);
                }
                return;
        }
        prev = cp;
    }
    clnt_destroy(clnt); /* non-cached */
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
