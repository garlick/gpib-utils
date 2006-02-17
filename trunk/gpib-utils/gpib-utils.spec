Name: gpib-utils
Version: 1.1
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
* HP 3455 digital voltmeter
* HP 3457 digital multi-meter
* Racal 5005 digital multi-meter
* HP 8656A syhthesized frequency generator

%prep
%setup

%build
make

%install
mkdir -p $RPM_BUILD_ROOT/{%{_bindir},%{_mandir}/man1}
BINDIR=$RPM_BUILD_ROOT/%{_bindir} MANDIR=$RPM_BUILD_ROOT/%{_mandir}/man1 make -e install

%clean

%files
%defattr(-,root,root)
%doc ChangeLog examples COPYING
%{_bindir}/*
%{_mandir}/man1/*

%changelog
* Fri Feb 17 2006 Jim Garlick <garlick@speakeasy.net>
- Update for Fedora spec style.

* Tue Jul 11 2005 Jim Garlick <garlick@speakeasy.net>
- Release 1.1-1

* Tue Jul 11 2005 Jim Garlick <garlick@speakeasy.net>
- Repackage for sourceforge.
- Release 0.2-1
