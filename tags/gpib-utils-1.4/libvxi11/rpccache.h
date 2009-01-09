/* rpccache.c - cache RPC connections for reuse */

CLIENT *      clnt_create_cached(char *host, u_long prog, u_long vers, 
                                 char *proto);

CLIENT *      clnttcp_create_cached(struct sockaddr_in *addr, u_long prog, 
                                    u_long vers, int *sockp, u_int sendsz, 
                                    u_int recvsz);

void          clnt_destroy_cached(CLIENT *clnt);

void          set_rpccache_debug(int doDebug);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
