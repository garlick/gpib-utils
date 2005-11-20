Name: gpib-utils
Version: 1.1
Release: 1
Summary: GPIB Instrument Utilities
Group: Application/Engineering
License:  GPL
Packager: Jim Garlick <garlick@speakeasy.net>
BuildRequires: linux-gpib
Requires: linux-gpib

# Disable automatic binary strip and debug packages.
%define debug_package %{nil}
%define __spec_install_post /usr/lib/rpm/brp-compress || :

BuildRoot: %{_tmppath}/%{name}-%{version}

Source0: gpib-utils-%{version}.tar.bz2

%description
GPIB Instrument utilities for:
* HP 1630/1631 logic analyzer
* HP 3455 digital voltmeter
* HP 3457 digital multi-meter
* Racal 5005 digital multi-meter
* HP 8656A syhthesized frequency generator

%prep
%setup -q -n gpib-utils-%{version}

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man1
make BINDIR=$RPM_BUILD_ROOT/usr/bin MANDIR=$RPM_BUILD_ROOT/usr/share/man/man1 install

%clean

%files
%defattr(-,root,root,-)
%doc ChangeLog
%doc examples
/usr/bin/*
/usr/share/man/man1/*

%changelog
* Tue Jul 11 2005 Jim Garlick <garlick@llnl.gov>
- Release 1.1-1

* Tue Jul 11 2005 Jim Garlick <garlick@llnl.gov>
- Repackage for sourceforge.
- Release 0.2-1

* Tue Jul 11 2005 Jim Garlick <garlick@llnl.gov>
- Added hptodino program
- Release 0.1-4

* Tue Apr 09 2005 Jim Garlick <garlick@llnl.gov>
- Added r5005 program
- Release 0.1-3

* Tue Apr 07 2005 Jim Garlick <garlick@llnl.gov>
- Include hptopbm and make hp1630 and hp3455 setgid gpib
- Release 0.1-2

* Tue Apr 07 2005 Jim Garlick <garlick@llnl.gov>
- First attempt at packaging gpib-utils.
- Release 0.1-1
