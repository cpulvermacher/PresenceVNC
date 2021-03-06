# Note that this is NOT a relocatable package
Name: LibVNCServer
Version: 0.9.7
Release: 2
Summary: a library to make writing a vnc server easy
Copyright: GPL
Group: Libraries/Network
Packager: Johannes.Schindelin <Johannes.Schindelin@gmx.de>
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

%description
LibVNCServer makes writing a VNC server (or more correctly, a program
exporting a framebuffer via the Remote Frame Buffer protocol) easy.

It is based on OSXvnc, which in turn is based on the original Xvnc by
ORL, later AT&T research labs in UK.

It hides the programmer from the tedious task of managing clients and
compression schemata.

LibVNCServer was put together and is (actively ;-) maintained by
Johannes Schindelin <Johannes.Schindelin@gmx.de>

%package devel
Requires:     %{name} = %{version}
Summary:      Static Libraries and Header Files for LibVNCServer
Group:        Libraries/Network
Requires:     %{name} = %{version}

%description devel
Static Libraries and Header Files for LibVNCServer.

%package x11vnc
Requires:     %{name} = %{version}
Summary:      VNC server for the current X11 session
Group:        User Interface/X
Requires:     %{name} = %{version}

%description x11vnc
x11vnc is to X Window System what WinVNC is to Windows, i.e. a server
which serves the current X Window System desktop via RFB (VNC)
protocol to the user.

Based on the ideas of x0rfbserver and on LibVNCServer, it has evolved
into a versatile and performant while still easy to use program.

%prep
%setup -n %{name}-%{version}

%build
# CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix}
%configure
make

%install
[ -n "%{buildroot}" -a "%{buildroot}" != / ] && rm -rf %{buildroot}
# make install prefix=%{buildroot}%{_prefix}
%makeinstall includedir="%{buildroot}%{_includedir}/rfb"

%{__install} -d -m0755 %{buildroot}%{_datadir}/x11vnc/classes
%{__install} classes/VncViewer.jar classes/index.vnc \
  %{buildroot}%{_datadir}/x11vnc/classes

%clean
[ -n "%{buildroot}" -a "%{buildroot}" != / ] && rm -rf %{buildroot}

%pre
%post
%preun
%postun

%files
%defattr(-,root,root)
%doc README INSTALL AUTHORS ChangeLog NEWS TODO 
%{_bindir}/LinuxVNC
%{_bindir}/libvncserver-config
%{_libdir}/libvncclient.*
%{_libdir}/libvncserver.*

%files devel
%defattr(-,root,root)
%{_includedir}/rfb/*

%files x11vnc
%defattr(-,root,root)
%{_bindir}/x11vnc
%{_mandir}/man1/x11vnc.1*
%{_datadir}/x11vnc/classes

%changelog
* Fri Aug 19 2005 Alberto Lusiani <alusiani@gmail.com> release 2
- create separate package for x11vnc to prevent conflicts with x11vnc rpm
- create devel package, needed to compile but not needed for running
* Sun Feb 9 2003 Johannes Schindelin
- created libvncserver.spec.in

