typedef struct ics_struct *ics_t;

/* Get/set the current VXI-11 logical interface name.
 * Caller must free the string returned by the get function.
 */
int     ics_get_interface_name(ics_t ics, char **strp);
int     ics_set_interface_name(ics_t ics, char *str);

/* Force reload of default config.
 */
int     ics_reload_config(ics_t ics);

/* Reload factory config settings.
 */
int     ics_reload_factory(ics_t ics);

/* Commit (write) current config.
 */
int     ics_commit_config(ics_t ics);

/* Reboot the ics device.
 */
int     ics_reboot(ics_t ics);

/* Obtain the device identity string, which contains the FW revision, 
 * ICS product model number, etc. Caller must free the identity string.
 */
int     ics_idn_string(ics_t ics, char **strp);

/* Get the current contents of the error log.  Put a copy of the array
 * in 'errp' and its length in 'countp'.  Caller must free the error log.
 */
int     ics_error_logger(ics_t ics, unsigned int **errp, int *countp);

/* Initialize/finalize the ics module.
 */
ics_t   ics_init (char *host);
void    ics_fini(ics_t ics);

/* Mapping of error numbers (from the error log) to descriptive text.
 */
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
