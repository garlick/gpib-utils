struct Device_SrqParms {
   opaque handle<>;
};

program DEVICE_INTR {
   version DEVICE_INTR_VERSION {
      void device_intr_srq (Device_SrqParms) = 30;
   }=1;
}= 0x0607B1;
