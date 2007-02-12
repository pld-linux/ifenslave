Summary:	Configure network interfaces for parallel routing
Summary(pl.UTF-8):   Konfiguracja interfejsów sieciowych dla równoległego routingu
Summary(pt_BR.UTF-8):   Configura interfaces de rede para roteamento paralelo
Name:		ifenslave
Version:	1.1.0
Release:	1
License:	GPL
Group:		Networking
Source0:	%{name}.c
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%define		_sbindir	/sbin

%description
ifenslave configures network interfaces for parallel routing.

%description -l pl.UTF-8
ifenslave konfiguruje interfejsy sieciowe dla równoległego routingu.

%description -l pt_BR.UTF-8
ifenslave configura interfaces de rede para roteamento paralelo.

%prep

%build
%{__cc} %{rpmcflags} -o %{name} %{SOURCE0}

%install
rm -rf $RPM_BUILD_ROOT
install -D %{name} $RPM_BUILD_ROOT%{_sbindir}/%{name}

%clean
rm -fr $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%attr(755,root,root) %{_sbindir}/ifenslave
