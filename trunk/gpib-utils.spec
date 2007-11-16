Name: gpib-utils
Version: 1.2
Release: 1
Summary: GPIB Instrument Utilities
License:  GPL
Group: Application/Engineering
Url: http://sourceforge.net/projects/gpib-utils/
BuildRequires: linux-gpib
Requires: linux-gpib
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root-%(%{__id_u} -n)

%description
GPIB Instrument utilities for:
* HP 1630/1631 logic analyzer
* HP 3455 digital multi-meter
* HP 3457 digital multi-meter
* Racal 5005 digital multi-meter
* HP 8656A synthesized frequency generator
* HP 3488A switch/control unit
* Stanford Research DG535 four channel digital delay/pulse generator
* Stanford Research SR620 universal time interval counter
* Agilent 603xA family autoranging system DC power supplies
* ICS 8065 VXI-to-GPIB gateway
%prep
%setup

%build
make

%install
mkdir -p $RPM_BUILD_ROOT/{%{_bindir},%{_mandir}/man1,%{_sysconfdir}}
BINDIR=$RPM_BUILD_ROOT/%{_bindir} MANDIR=$RPM_BUILD_ROOT/%{_mandir}/man1 make -e install
cp etc/gpib-utils.conf $RPM_BUILD_ROOT/%{_sysconfdir}/

%clean

%files
%defattr(-,root,root)
%doc ChangeLog COPYING INSTALL README README.vxi
# %doc examples 
%{_bindir}/*
%{_mandir}/man1/*
%config %{_sysconfdir}/gpib-utils.conf

%changelog
* Fri Feb 17 2006 Jim Garlick <garlick@speakeasy.net>
- Update for Fedora spec style.

* Tue Jul 11 2005 Jim Garlick <garlick@speakeasy.net>
- Release 1.1-1

* Tue Jul 11 2005 Jim Garlick <garlick@speakeasy.net>
- Repackage for sourceforge.
- Release 0.2-1
