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

static int rpccache_debug = 0;

#define CLNT_CACHE_MAGIC 0x346abefa
struct clnt_cache_struct {
    int magic;
    enum { CLNT_CREATE, CLNTTCP_CREATE } type;
    union { 
        struct clnt_create_parms {
            char host[MAXHOSTNAMELEN];
            char proto[MAXHOSTNAMELEN];
            u_long prog;
            u_long vers;
        } c;
        struct clnttcp_create_parms {
            struct sockaddr_in addr;
            int sock;
            u_int sendsz;
            u_int recvsz;
            u_long prog;
            u_long vers;
        } t;
    } u;
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
        if (cp->type == CLNT_CREATE
                && !strcmp(cp->u.c.host, host) 
                && !strcmp(cp->u.c.proto, proto) 
                && cp->u.c.prog == prog && cp->u.c.vers == vers) {
            cp->usecount++;
            if (rpccache_debug)
                fprintf(stderr, "XXX clnt_create_cached = %p (count=%d)\n", 
                        cp->clnt, cp->usecount);
            return cp->clnt;
        }
    }
    if ((clnt = clnt_create(host, prog, vers, proto))) {
        if ((new = malloc(sizeof(struct clnt_cache_struct)))) {
            new->magic = CLNT_CACHE_MAGIC;
            new->type = CLNT_CREATE;
            strncpy(new->u.c.host, host, MAXHOSTNAMELEN);
            new->u.c.host[MAXHOSTNAMELEN - 1] = '\0';
            strncpy(new->u.c.proto, proto, MAXHOSTNAMELEN);
            new->u.c.proto[MAXHOSTNAMELEN - 1] = '\0';
            new->u.c.prog = prog;
            new->u.c.vers = vers;
            new->clnt = clnt;
            new->usecount = 1;
            new->next = clnt_cache;
            clnt_cache = new;
            if (rpccache_debug)
                fprintf(stderr, "XXX clnt_create_cached = %p (new)\n", clnt);
        }
    } else
        if (rpccache_debug)
            fprintf(stderr, "XXX clnt_create_cached = NULL\n");
    return clnt;
}

CLIENT *
clnttcp_create_cached(struct sockaddr_in *addr, u_long prog, u_long vers,
                      int *sockp, u_int sendsz, u_int recvsz)
{
    CLIENT *clnt;
    struct clnt_cache_struct *cp, *new;
    int savesock = *sockp;

    for (cp = clnt_cache; cp != NULL; cp = cp->next) {
        assert(cp->magic == CLNT_CACHE_MAGIC);
        if (cp->type == CLNTTCP_CREATE
                && cp->u.t.addr.sin_port        == addr->sin_port
                && cp->u.t.addr.sin_addr.s_addr == addr->sin_addr.s_addr
                && cp->u.t.sock == savesock
                && cp->u.t.sendsz == sendsz && cp->u.t.recvsz == recvsz
                && cp->u.t.prog == prog && cp->u.t.vers == vers) {
            cp->usecount++;
            if (rpccache_debug)
                fprintf(stderr, "XXX clnttcp_create_cached (addr=%s:%d, ...) "
                        " = %p (count=%d)\n", inet_ntoa(addr->sin_addr), 
                        ntohs(addr->sin_port), cp->clnt, cp->usecount);
            return cp->clnt;
        }
    }
    if ((clnt = clnttcp_create(addr, prog, vers, sockp, sendsz, recvsz))) {
        if ((new = malloc(sizeof(struct clnt_cache_struct)))) {
            new->magic = CLNT_CACHE_MAGIC;
            new->type = CLNTTCP_CREATE;
            new->u.t.addr.sin_port         = addr->sin_port;
            new->u.t.addr.sin_addr.s_addr  = addr->sin_addr.s_addr;
            new->u.t.sock = savesock;
            new->u.t.sendsz = sendsz;
            new->u.t.recvsz = recvsz;
            new->u.t.prog = prog;
            new->u.t.vers = vers;
            new->clnt = clnt;
            new->usecount = 1;
            new->next = clnt_cache;
            clnt_cache = new;
            if (rpccache_debug)
                fprintf(stderr, "XXX clnttcp_create_cached (addr %s:%d, ...) "
                        "= %p (new)\n", inet_ntoa(addr->sin_addr), 
                        htons(addr->sin_port), clnt);
        }
    } else
        if (rpccache_debug)
            fprintf(stderr, "XXX clnttcp_create_cached (addr %s:%d, ...) = "
                    "NULL\n", inet_ntoa(addr->sin_addr), htons(addr->sin_port));
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
                    if (rpccache_debug)
                        printf("XXX clnt_destroy_cached (%p) (last use)\n", 
                                clnt);
                    clnt_destroy(clnt);
                    if (prev == NULL)
                        clnt_cache = cp->next;
                    else
                        prev->next = cp->next;
                    memset(cp, 0, sizeof(struct clnt_cache_struct));
                    free(cp);
                } else
                    if (rpccache_debug)
                        printf("XXX clnt_destroy_cached (%p) (count=%d)\n", 
                                cp->clnt, cp->usecount);
                return;
        }
        prev = cp;
    }
    if (rpccache_debug)
        fprintf(stderr, "XXX clnt_destroy_cached (%p) (uncached)\n", clnt);
    clnt_destroy(clnt); /* non-cached */
}

void
set_rpccache_debug(int doDebug)
{
    rpccache_debug = doDebug;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
