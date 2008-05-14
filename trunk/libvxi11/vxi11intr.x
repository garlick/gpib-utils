/* VXI-11 RPCL definitions. Taken from appendix C of the VXI-11 specfication. */
/* See http://www.vxi.org */

%#include <strings.h>
%#include <stdlib.h>

%#define VXI11_INTR_SVC_PROGNUM   395185
%#define VXI11_INTR_SVC_VERSION   1

%void device_intr_1(struct svc_req *rqstp, register SVCXPRT *transp);


struct Device_SrqParms {
   opaque handle<>;
};

program DEVICE_INTR {
   version DEVICE_INTR_VERSION {
      void device_intr_srq (Device_SrqParms) = 30;
   }=1;
}= 0x0607B1;
