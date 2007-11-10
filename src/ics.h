typedef struct ics_struct *ics_t;

ics_t       ics_init (char *host);
void        ics_fini(ics_t ics);

int         ics_reboot(ics_t ics);
int         ics_errorlogger(ics_t ics, unsigned int *errs, int size, int *len);
int         ics_idnreply(ics_t ics, char *str, int len);

#define ICS_ERRLOG { \
    { 1,    "VXI-11 syntax error" }, \
    { 3,    "GPIB device not accessible" }, \
    { 4,    "invalid VXI-11 link ID" }, \
    { 5,    "VXI-11 parameter error" }, \
    { 6,    "invalid VXI-11 channel, channel not established" }, \
    { 8,    "invalid VXI-11 operation" }, \
    { 9,    "insufficient resources" }, \
    { 11,   "device locked by another link ID" }, \
    { 12,   "device not locked" }, \
    { 15,   "I/O timeout error" }, \
    { 17,   "I/O error" }, \
    { 21,   "invalid GPIB address specified" }, \
    { 23,   "operation aborted (indicator, not a true error)" }, \
    { 29,   "channel already established" }, \
    { 60,   "channel not active" }, \
    {110,   "device already locked" }, \
    {111,   "timeout attempting to gain a device lock" }, \
    {999,   "unspecified/unknown error" }, \
    {1000,  "VXI-11 protocol error" }, \
    {2000,  "RPC protocol error" }, \
    {2001,  "unsupported RPC function" }, \
    {2002,  "insufficient RPC message length" }, \
    {0,     NULL}, \
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

