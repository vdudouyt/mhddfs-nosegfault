#    mhddfs - Multi HDD [FUSE] File System
#    Copyright (C) 2008 Dmitry E. Oboukhov <dimka@avanto.org>

#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.

#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.


#-------------------------------------------------------------------------------
# The all-important N-V-R, Name-Version-Release.
#-------------------------------------------------------------------------------

Name: mhddfs
Version: %version
Release: %{release}

#-------------------------------------------------------------------------------
# Additional header information.
#-------------------------------------------------------------------------------

Summary: A fuse-based file system for unifying several mount points into one.
License: GPLv3
Group: File tools
URL: http://mhddfs.uvw.ru
Packager: MHDDFS Team <unera at debian.org>

Source: %{name}_%version.tar.gz
Requires: fuse, fuse-libs
BuildRoot: %{_tmppath}/%{name}-buildroot

%description
This FUSE-based file system allows mount points (or directories) to be
combined, simulating a single big volume which can merge several hard
drives or remote file systems. It is like unionfs, but can choose the
drive with the most free space to create new files on, and can move
data transparently between drives.

#===============================================================================

%prep
%setup -q

%build
    make

%install
    rm -rf $RPM_BUILD_ROOT
    mkdir -p $RPM_BUILD_ROOT/usr/bin
    cp %{name} $RPM_BUILD_ROOT/usr/bin/%{name}
    mkdir -p $RPM_BUILD_ROOT/usr/share/man/man1
    cp %{name}.1 $RPM_BUILD_ROOT/usr/share/man/man1

%clean
    rm -rf $RPM_BUILD_ROOT
    #make clean

#===============================================================================

#-------------------------------------------------------------------------------
# The following sections control the uninstallation process. Since we never
# uninstall this package, these sections are empty.
#-------------------------------------------------------------------------------

%preun
    exit 0

%postun
    exit 0

#===============================================================================

#-------------------------------------------------------------------------------
# The script in this section gains control just before installation.
#-------------------------------------------------------------------------------

%pre
    exit 0

#===============================================================================

#-------------------------------------------------------------------------------
# The script in this section gains control just after installation.
#-------------------------------------------------------------------------------

%post
    exit 0

#===============================================================================

#-------------------------------------------------------------------------------
# This section describes the files in this package.
#-------------------------------------------------------------------------------

%files
%defattr(-,root,root)
%doc README* ChangeLog LICENSE COPYING mhddfs.1

/usr/bin/mhddfs
/usr/share/man/man1/mhddfs.1.gz

# emacs is your friend
# local variables:
# mode: rpm-spec
# end:

