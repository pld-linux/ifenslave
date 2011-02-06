Summary:	Configure network interfaces for parallel routing
Summary(pl.UTF-8):	Konfiguracja interfejsów sieciowych dla równoległego routingu
Summary(pt_BR.UTF-8):	Configura interfaces de rede para roteamento paralelo
Name:		ifenslave
Version:	1.1.0
Release:	2
License:	GPL
Group:		Networking
Source0:	%{name}.c
Patch0:		%{name}-debian.patch
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%define		_sbindir	/sbin

%description
Attach and detach slave interfaces to a bonding device. This is a tool to
attach and detach slave network interfaces to a bonding device. A bonding
device will act like a normal Ethernet network device to the kernel, but
will send out the packets via the slave devices using a simple round-robin
scheduler. This allows for simple load-balancing, identical to "channel
bonding" or "trunking" techniques used in switches.

%description -l pl.UTF-8
ifenslave konfiguruje interfejsy sieciowe dla równoległego routingu.

%description -l pt_BR.UTF-8
ifenslave configura interfaces de rede para roteamento paralelo.

%prep
%setup -Tc -q
install %{SOURCE0} .
%patch0 -p1


%build
%{__cc} %{rpmcflags} -o %{name} %{name}.c

%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT{%{_sbindir},%{_mandir}/man8}

install %{name} $RPM_BUILD_ROOT%{_sbindir}
install ifenslave.8 $RPM_BUILD_ROOT%{_mandir}/man8

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%attr(755,root,root) %{_sbindir}/%{name}
%{_mandir}/man8/*.8*
