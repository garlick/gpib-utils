Name: gpib-utils
Version: 1.5
Release: pre3

Summary: GPIB Instrument Utilities
License:  GPL
Group: Application/Engineering
Url: http://sourceforge.net/projects/gpib-utils/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Requires: %{name}-libs = %{version}-%{release}

%if 0
%define _with_linuxgpib 1
%endif

%if 0%{?_with_linuxgpib}
BuildRequires: linux-gpib
%endif

%package devel
Requires: %{name} = %{version}-%{release}
Summary: Headers and libraries for developing VXI-11 applications.
Group: Development/Libraries

%package libs
Requires: %{name} = %{version}-%{release}
Summary: Libraries for VXI-11 applications.
Group: System Environment/Libraries

%description
Utilities for controlling various electronic instruments over GPIB.

%description devel
Header files and static library for developing VXI-11 applications.

%description libs
A shared library for VXI-11 applications.

%prep
%setup

%build
%configure \
  %{?_with_linuxgpib: --with-linux-gpib}
make

%check
make check

%install
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post libs
if [ -x /sbin/ldconfig ]; then /sbin/ldconfig %{_libdir}; fi

%postun libs
if [ -x /sbin/ldconfig ]; then /sbin/ldconfig %{_libdir}; fi

%files
%defattr(-,root,root,0755)
%doc ChangeLog COPYING
%{_bindir}/*
%{_mandir}/man1/*
%{_mandir}/man5/*
%config(noreplace) %{_sysconfdir}/gpib-utils.conf

%files devel
%defattr(-,root,root,0755)
%{_includedir}/*
%{_libdir}/*.la
%{_libdir}/*.a
%{_libdir}/*.so
%{_libdir}/pkgconfig/*

%files libs
%defattr(-,root,root,0755)
%{_libdir}/*.so.*
