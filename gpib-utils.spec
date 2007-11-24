# set to 1 if building with linux-gpib
%define buildgpib 0

Name: gpib-utils
Version: 1.2
Release: 1
Summary: GPIB Instrument Utilities
License:  GPL
Group: Application/Engineering
Url: http://sourceforge.net/projects/gpib-utils/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root-%(%{__id_u} -n)
%if %{buildgpib}
Requires: linux-gpib
%endif

%description
GPIB Instrument utilities for various electronic instruments.

%prep
%setup

%build
%if %{buildgpib}
make all-gpib
%else
make all-nogpib
%endif

%install
mkdir -p $RPM_BUILD_ROOT/{%{_bindir},%{_mandir}/man1,%{_sysconfdir}}
make -e install \
   BINDIR=$RPM_BUILD_ROOT/%{_bindir} MANDIR=$RPM_BUILD_ROOT/%{_mandir}/man1 
cp etc/gpib-utils.conf $RPM_BUILD_ROOT/%{_sysconfdir}/

%clean

%files
%defattr(-,root,root)
%doc ChangeLog COPYING
# %doc examples 
%{_bindir}/*
%{_mandir}/man1/*
%config %{_sysconfdir}/gpib-utils.conf
