#define VXI11_REOS 	0x0400
#define VXI11_XEOS 	0x0800
#define VXI11_BIN 	0x1000

#define VXI11_TNONE	0
#define VXI11_T10us	1
#define VXI11_T30us	2
#define VXI11_T100us	3
#define VXI11_T300us	4
#define VXI11_T1ms	5
#define VXI11_T3ms	6
#define VXI11_T10ms	7
#define VXI11_T30ms	8
#define VXI11_T100ms	9
#define VXI11_T300ms	10
#define VXI11_T1s	11
#define VXI11_T3s	12
#define VXI11_T10s	13
#define VXI11_T30s	14
#define VXI11_T100s	15
#define VXI11_T300s	16
#define VXI11_T1000s	17

int vxi11_ibask(int ud, int option, int *result);
int vxi11_ibbna(int ud, const char *name);
int vxi11_ibcac(int ud, int synchronous);
int vxi11_ibclr(int ud);
int vxi11_ibcmd(int ud, const void *commands, long num_bytes);
int vxi11_ibcmda(int ud, const void *commands, long num_bytes);
int vxi11_ibconfig(int ud, int option, int setting);
int vxi11_ibdev(int board_index, int pad, int sad, int timeout, 
                int send_eoi, int eos);
int vxi11_ibeos(int ud, int eosmode);
int vxi11_ibeot(int ud, int send_eoi);
int vxi11_ibevent(int ud, short *event);
int vxi11_ibfind(const char *name);
int vxi11_ibgts(int ud, int shadow_handshake);
int vxi11_ibist(int ud, int ist);
int vxi11_iblines(int ud, short *line_status);
int vxi11_ibln(int ud, int sad, short *found_listener);
int vxi11_ibloc(int ud);
int vxi11_ibonl(int ud, int online);
int vxi11_ibpad(int ud, int pad);
int vxi11_ibpct(int ud);
int vxi11_ibppc(int ud, int configuration);
int vxi11_ibrd(int ud, void *buffer, long num_bytes);
int vxi11_ibrda(int ud, void *buffer, long num_bytes);
int vxi11_ibrdf(int ud, const char *file_path);
int vxi11_ibrpp(int ud, char *ppoll_result);
int vxi11_ibrsc(int ud, int request_control);
int vxi11_ibrsp(int ud, char *result);
int vxi11_ibrsv(int ud, int status_byte);
int vxi11_ibsad(int ud, int sad);
int vxi11_ibsic(int ud);
int vxi11_ibsre(int ud, int enable);
int vxi11_ibstop(int ud);
int vxi11_ibtmo(int ud, int timout);
int vxi11_ibtrg(int ud);
int vxi11_ibwait(int ud, int status_mask);
int vxi11_ibwrt(int ud, const void *data, long num_bytes);
int vxi11_ibwrta(int ud, const void *data, long num_bytes);
int vxi11_ibwrtf(int ud, const char *file_path);

#if NI_488_2_NAMES
#define REOS	VXI11_REOS
#define XEOS	VXI11_XEOS
#define BIN	VXI11_BIN

#define TNONE	VXI11_TNONE
#define T10us	VXI11_T10us
#define T30us	VXI11_T30us
#define T100us	VXI11_T100us
#define T300us	VXI11_T300us
#define T1ms	VXI11_T1ms
#define T3ms	VXI11_T3ms
#define T10ms	VXI11_T10ms
#define T30ms	VXI11_T30ms
#define T100ms	VXI11_T100ms
#define T300ms	VXI11_T300ms
#define T1s	VXI11_T1s
#define T3s	VXI11_T3s
#define T10s	VXI11_T10s
#define T30s	VXI11_T30s
#define T100s	VXI11_T100s
#define T300s	VXI11_T300s
#define T1000s	VXI11_T1000s

#define ibask	vxi11_ibask
#define ibbna	vxi11_ibbna
#define ibcac	vxi11_ibcac
#define ibclr   vxi11_ibclr
#define ibcmd	vxi11_ibcmd
#define ibcmda	vxi11_ibcmda
#define ibconfig vxi11_ibconfig
#define ibdev   vxi11_ibdev
#define ibeos   vxi11_ibeos
#define ibeot	vxi11_ibeot
#define ibevent	vxi11_ibevent
#define ibfind	vxi11_ibfind
#define ibgts	vxi11_ibgts
#define ibist	vxi11_ibist
#define iblines	vxi11_iblines
#define ibln    vxi11_ibln
#define ibloc   vxi11_ibloc
#define ibonl   vxi11_ibonl
#define ibpad   vxi11_ibpad
#define ibpct	vxi11_ibpct
#define ibppc   vxi11_ibppc
#define ibrd	vxi11_ibrd
#define ibrda	vxi11_ibrda
#define ibrdf	vxi11_ibrdf
#define ibrpp	vxi11_ibrpp
#define ibrsc	vxi11_ibrsc
#define ibrsp	vxi11_ibrsp
#define ibrsv	vxi11_ibrsv
#define ibsad	vxi11_ibsad
#define ibsic	vxi11_ibsic
#define ibsre	vxi11_ibsre
#define ibstop	vxi11_ibstop
#define ibtmo	vxi11_ibtmo
#define ibtrg	vxi11_ibtrg
#define ibwait	vxi11_ibwait
#define ibwrt	vxi11_ibwrt
#define ibwrta	vxi11_ibwrta
#define ibwrtf	vxi11_ibwrtf
#endif
