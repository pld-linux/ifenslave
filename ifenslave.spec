Summary:	Configure network interfaces for parallel routing
Summary(pl):	Konfiguracja interfejsów sieciowych dla równoleg³ego routingu
Summary(pt_BR):	Configura interfaces de rede para roteamento paralelo
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

%description -l pl
ifenslave konfiguruje interfejsy sieciowe dla równoleg³ego routingu.

%description -l pt_BR
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
