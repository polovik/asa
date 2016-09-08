#
# spec file for package asa
#
# Copyright (c) 2016 Dmitry Valento <decobramegra@gmail.com>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#
Name:           asa
Version:        1.0.0
Release:        0
Group:          Productivity/Scientific/Electronics
License:        MIT
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
#BuildArch:      noarch
%if 0%{?suse_version}
BuildRequires:  gcc-c++ libstdc++-devel libqt5-qtbase-devel libqt5-qtmultimedia-devel libqt5-linguist update-desktop-files
%else
BuildRequires:  gcc-c++ libstdc++-devel qt5-qtbase-devel qt5-qtmultimedia-devel qt5-linguist
%endif
Source:         %{name}_%{version}.orig.tar.gz
#Source0:        https://github.com/polovik/%{name}/archive/v%{version}.tar.gz#/%{name}-%{version}.tar.gz
Summary:        Analog signature analyzer

%description
This utility is used for create signatures for black boxes on pins and then match with another boxes.

It needs to create additianal schematic to perform diagnose.

%global debug_package %{nil}
%define _binaries_in_noarch_packages_terminate_build   0

%prep
%setup -q -c

%build
#mkdir build
#cd build
%ifarch x86_64
  export PATH=/usr/lib64/qt5/bin:$PATH
#  export QTDIR=/usr/lib64/qt5/
%else
  export PATH=/usr/lib/qt5/bin:$PATH
#  export QTDIR=/usr/lib/qt5/
%endif
qmake
make

%install
rm -Rf "%buildroot";
mkdir "%buildroot";
make INSTALL_ROOT="%buildroot" install
%if 0%{?suse_version}
%suse_update_desktop_file -u -r -G 'Analog signature analyzer' %{buildroot}%{_datadir}/applications/%{name}.desktop Science Engineering
%endif

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%{_bindir}/*
%{_datadir}/*

#%changelog

