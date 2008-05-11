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
GPIB Instrument utilities for various electronic instruments.

%prep
%setup

%build
%configure
make

%install
mkdir -p $RPM_BUILD_ROOT/{%{_bindir},%{_mandir}/man1,%{_sysconfdir}}
make install DESTDIR=$RPM_BUILD_ROOT

%clean

%files
%defattr(-,root,root)
%doc ChangeLog COPYING
%doc examples 
%{_bindir}/*
%{_mandir}/man1/*
#%config %{_sysconfdir}/gpib-utils.conf
