int vxi11_ibask(int ud, int option, int *result)
{
}

int vxi11_ibbna(int ud, const char *name)
{
}

int vxi11_ibcac(int ud, int synchronous)
{
}

int vxi11_ibclr(int ud)
{
}

int vxi11_ibcmd(int ud, const void *commands, long num_bytes)
{
}

int vxi11_ibcmda(int ud, const void *commands, long num_bytes)
{
}

int vxi11_ibconfig(int ud, int option, int setting)
{
}

int vxi11_ibdev(int board_index, int pad, int sad, int timeout, 
                int send_eoi, int eos)
{
}

int vxi11_ibeos(int ud, int eosmode)
{
}

int vxi11_ibeot(int ud, int send_eoi)
{
}

int vxi11_ibevent(int ud, short *event)
{
}

int vxi11_ibfind(const char *name)
{
}

int vxi11_ibgts(int ud, int shadow_handshake)
{
}

int vxi11_ibist(int ud, int ist)
{
}

int vxi11_iblines(int ud, short *line_status)
{
}

int vxi11_ibln(int ud, int sad, short *found_listener)
{
}

int vxi11_ibloc(int ud)
{
}

int vxi11_ibonl(int ud, int online)
{
}

int vxi11_ibpad(int ud, int pad)
{
}

int vxi11_ibpct(int ud)
{
}

int vxi11_ibppc(int ud, int configuration)
{
}

int vxi11_ibrd(int ud, void *buffer, long num_bytes)
{
}

int vxi11_ibrda(int ud, void *buffer, long num_bytes)
{
}

int vxi11_ibrdf(int ud, const char *file_path)
{
}

int vxi11_ibrpp(int ud, char *ppoll_result)
{
}

int vxi11_ibrsc(int ud, int request_control)
{
}

int vxi11_ibrsp(int ud, char *result)
{
}

int vxi11_ibrsv(int ud, int status_byte)
{
}

int vxi11_ibsad(int ud, int sad)
{
}

int vxi11_ibsic(int ud)
{
}

int vxi11_ibsre(int ud, int enable)
{
}

int vxi11_ibstop(int ud)
{
}

int vxi11_ibtmo(int ud, int timout)
{
}

int vxi11_ibtrg(int ud)
{
}

int vxi11_ibwait(int ud, int status_mask)
{
}

int vxi11_ibwrt(int ud, const void *data, long num_bytes)
{
}

int vxi11_ibwrta(int ud, const void *data, long num_bytes)
{
}

int vxi11_ibwrtf(int ud, const char *file_path)
{
}

