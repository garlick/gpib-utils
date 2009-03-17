Name: gpib-utils
Version: 1.3
Release: 1
Summary: GPIB Instrument Utilities
License:  GPL
Group: Application/Engineering
Url: http://sourceforge.net/projects/gpib-utils/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root-%(%{__id_u} -n)
BuildRequires: linux-gpib

%description
Utilities for controlling various electronic instruments over GPIB.

%prep
%setup

%build
%configure --with-linux-gpib
make

%install
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}
cp etc/gpib-utils.conf $RPM_BUILD_ROOT/%{_sysconfdir}/gpib-utils.conf

%clean

%files
%defattr(-,root,root)
%doc ChangeLog COPYING
%{_bindir}/*
%{_mandir}/man1/*
%{_mandir}/man5/*
%config %{_sysconfdir}/gpib-utils.conf
