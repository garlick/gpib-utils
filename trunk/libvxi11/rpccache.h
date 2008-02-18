/* rpccache.c - cache RPC connections for reuse */

CLIENT *clnt_create_cached(char *host, u_long prog, u_long vers, char *proto);
void clnt_destroy_cached(CLIENT *clnt);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
