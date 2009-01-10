/* ICS 8000 series RCPL from appendix A3 of "8065 Instruction Manual" rev 2 */

/* Rev 2 manual errata:
 * - procedure names must be lower case for RPCGEN (c.24, c.25)
 * - procedures with empty "Parms" arg really take void arg (c.24, c.25)
 * - change length+char[] to opaque (c.24)
 * - use procedure names in table B-1 (c.16. c.24, c.25)
 * - fw_revision (proc #24) is not implemented (table B-1)
 * - drop length member from interface_name and hostname structs (c.1, c.7)
 * - opaque data.len does not include trailing NULL for strings (b.1)
 * - ip's are unsigned int, not unsigned char[4] (c.9, c.10, c.11)
 */

/* IP-note: 4-byte IP's are packed into a 32 bit unsigned integer in
 * reverse network byte order.  XDR integers are in host byte order.
 */

%#include <strings.h>

/* Action parameter values */
%#define ICS_READ           0
%#define ICS_WRITE          1

/* The interface_name procedure is used to read/modify the current VXI-11
 * logical interface name.
 */
struct Int_Name_Parms {
   unsigned int action;
   opaque name<>;
};
struct Int_Name_Resp {
   unsigned int error;
   opaque name<>;
};

/* The rpc_port_number procedure is used to read/modify the TCP port used
 * by the RPC server.
 */
struct Rpc_Port_Parms {
   unsigned int action;
   unsigned int port;
};
struct Rpc_Port_Resp {
   unsigned int error;
   unsigned int port;
};

/* The core_port_number procedure is used to read/modify the TCP port
 * used by the VXI-11 core channel.
 */
struct Core_Port_Parms {
   unsigned int action;
   unsigned int port;
};
struct Core_Port_Resp {
   unsigned int error;
   unsigned int port;
};

/* The abort_port_number procedure is used to read/modify the TCP port
 * used by the VXI-11 abort channel.
 */
struct Abort_Port_Parms {
   unsigned int action;
   unsigned int port;
};
struct Abort_Port_Resp {
   unsigned int error;
   unsigned int port;
};

/* The config_port_number procedure is used to read/modify the TCP port
 * used by the Edevice configuration channel.
 */
struct Config_Port_Parms {
   unsigned int action;
   unsigned int port;
};
struct Config_Port_Resp {
   unsigned int error;
   unsigned int port;
};

/* The comm_timeout procedure is used to read/modfiy the TCP timeout value.
 * An inactive TCP channel will be left open this length of time before 
 * being closed.  A value of zero means no timeout checking.
 */
struct Comm_Timeout_Parms {
   unsigned int action;
   unsigned int timeout;
};
struct Comm_Timeout_Resp {
   unsigned int error;
   unsigned int timeout;
};

/* The hostname procedure is used to read/modify the hostname used by
 * the Edevice.  The hostname is only applicable if a dynamic DNS service
 * is available.
 */
struct Hostname_Parms {
   unsigned int action;
   opaque name<>;
};
struct Hostname_Resp {
   unsigned int error;
   opaque name<>;
};

/* The static_ip_mode procedure is used to read/modify the static IP mode.
 * If static_ip_mode is set TRUE, then the Edevice will use a static IP
 * and will need a netmask and gateway IP.
 */
struct Static_IP_Parms {
   unsigned int action;
   unsigned int mode;
};
struct Static_IP_Resp {
   unsigned int error;
   unsigned int mode;
};

/* The ip_number procedure is used to read/modify the static IP number.
 * If static_ip_mode is set TRUE, then the Edevice will use a static IP
 * (see the static_ip_mode function) and will need a netmask and gateway IP.
 */
struct IP_Number_Parms {
   unsigned int action;
   unsigned int ip; /* see IP-note above */
};
struct IP_Number_Resp {
   unsigned int error;
   unsigned int ip; /* see IP-note above */
};

/* The netmask procedure is used to read/modify the netmask.  
 * If static_ip_mode is set TRUE, then the Edevice will use a static IP
 * (see the static_ip_mode function) and will need a netmask and gateway IP.
 */
struct Netmask_Parms {
   unsigned int action;
   unsigned int ip; /* see IP-note above */
};
struct Netmask_Resp {
   unsigned int error;
   unsigned int ip; /* see IP-note above */
};

/* The gateway procedure is used to read/modify the gateway IP.  
 * If static_ip_mode is set TRUE, then the Edevice will use a static IP
 * (see the static_ip_mode function) and will need a netmask and gateway IP.
 */
struct Gateway_Parms {
   unsigned int action;
   unsigned int ip; /* see IP-note above */
};
struct Gateway_Resp {
   unsigned int error;
   unsigned int ip; /* see IP-note above */
};

/* The keepalive procedure is used to read/modify the keepalive value.
 * If set to zero, then keepalives will not be used.  If used, then this is
 * the time (in seconds) of inactivity prior to a keepalive being sent.
 */
struct Keepalive_Parms {
   unsigned int action;
   unsigned int time;
};
struct Keepalive_Resp {
   unsigned int error;
   unsigned int time;
};

/* The gpib_address procedure is used to read/modify the Edevice GPIB bus
 * address.
 */
struct Gpib_Addr_Parms {
   unsigned int action;
   unsigned int address;
};
struct Gpib_Addr_Resp {
   unsigned int error;
   unsigned int address;
};

/* The system_controller procedure is used to read/modify the system
 * controller mode.  If system controller mode is set to TRUE, then the
 * Edevice will initialize at boot time as the GPIB bus controller.
 */
struct Sys_Control_Parms {
   unsigned int action;
   unsigned int controller;
};
struct Sys_Control_Resp {
   unsigned int error;
   unsigned int controller;
};

/* The ren_mode procedure is used to read/modify the REN mode.  If the 
 * REN mode is TRUE, then REN will be asserted at boot time.
 */
struct Ren_Parms {
   unsigned int action;
   unsigned int ren;
};
struct Ren_Resp {
   unsigned int error;
   unsigned int ren;
};

/* The eos_8_bit_mode procedure is used to read/modify the 8-bit EOS
 * compare mode.  If the 8-bit compare mode is TRUE, then EOS compare
 * will be 8-bits.  If 8-bit compare mode is FALSE, then EOS compare
 * will be 7-bits.
 */
struct Eos_8bit_Parms {
   unsigned int action;
   unsigned int eos8bit;
};
struct Eos_8bit_Resp {
   unsigned int error;
   unsigned int eos8bit;
};

/* The auto_eos_mode procedure is used to read/modify the automatic EOS
 * on EOI mode.  If the autoEos mode is TRUE, then an EOS character will
 * be sent wtih EOI.
 */
struct Auto_Eos_Parms {
   unsigned int action;
   unsigned int autoEos;
};
struct Auto_Eos_Resp {
   unsigned int error;
   unsigned int autoEos;
};

/* The eos_active_mode procedure is used to read/modify the EOS active mode.
 * If the EOS mode is TRUE, then an EOS character will terminate reads. 
 */
struct Eos_Active_Parms {
   unsigned int action;
   unsigned int eosActive;
};
struct Eos_Active_Resp {
   unsigned int error;
   unsigned int eosActive;
};

/* The eos_char procedure is used to read/modify the EOS character.
 */
struct Eos_Char_Parms {
   unsigned int action;
   unsigned int eos;
};
struct Eos_Char_Resp {
   unsigned int error;
   unsigned int eos;
};

/* The reload_config procedure is used to cause a reload of the configuration
 * settings.  Any modified configuration settings will be restored to
 * default settings.
 */
struct Reload_Config_Resp {
   unsigned int error;
};

/* The commit_config procedure is used to cause the current configuration
 * settings to be saved.  Any modified configuration settings now become
 * default settings and will be reloaded as the default settings with either
 * reload_config or a reboot.
 */
struct Commit_Config_Resp {
   unsigned int error;
};

/* The reboot procedure is used to cause the Edevice to reboot.  This causes
 * all device links to be cleared, all connections closed, all resources 
 * released, and the default configuration to be loaded and used during
 * initialization.
 */
struct Reboot_Resp {
   unsigned int error;
};

/* The idn_string procedure is used to obtain a response similar to the GPIB
 * *IDN? string.  It contains the FW revision, the ICS product model number,
 * and ohter miscellaneous information.
 */
struct Idn_Resp {
   unsigned int error;
   opaque idn<>;
};

/* The error_logger procedure is used to obtain the current contents of the
 * error log.
 */
struct Error_Log_Resp {
   unsigned int error;
   unsigned int count;
   unsigned int errors[100];
};

program ICSCONFIG {
   version ICSCONFIG_VERSION {
      Int_Name_Resp interface_name (Int_Name_Parms) = 1;
      Rpc_Port_Resp rpc_port_number (Rpc_Port_Parms) = 2;
      Core_Port_Resp core_port_number (Core_Port_Parms) = 3;
      Abort_Port_Resp abort_port_number (Abort_Port_Parms) = 4;
      Config_Port_Resp config_port_number (Config_Port_Parms) = 5;
      Comm_Timeout_Resp comm_timeout (Comm_Timeout_Parms) = 6;
      Hostname_Resp hostname (Hostname_Parms) = 7;
      Static_IP_Resp static_ip_mode (Static_IP_Parms) = 8;
      IP_Number_Resp ip_number (IP_Number_Parms) = 9;
      Netmask_Resp netmask (Netmask_Parms) = 10;
      Gateway_Resp gateway (Gateway_Parms) = 11;
      Keepalive_Resp keepalive (Keepalive_Parms) = 12;
      Gpib_Addr_Resp gpib_address (Gpib_Addr_Parms) = 13;
      Sys_Control_Resp system_controller (Sys_Control_Parms) = 14;
      Ren_Resp ren_mode (Ren_Parms) = 15;
      Eos_8bit_Resp eos_8_bit_mode (Eos_8bit_Parms) = 16;
      Auto_Eos_Resp auto_eos_mode (Auto_Eos_Parms) = 17;
      Eos_Active_Resp eos_active (Eos_Active_Parms) = 18;
      Eos_Char_Resp eos_char (Eos_Char_Parms) = 19;
      Reload_Config_Resp reload_config (void) = 20;
      Commit_Config_Resp commit_config (void) = 22;
      Reboot_Resp reboot (void) = 23;
      Idn_Resp idn_string (void) = 25;
      Error_Log_Resp error_logger (void) = 26;
   } = 1;
} = 1515151515;

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
