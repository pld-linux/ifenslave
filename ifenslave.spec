Summary:	Configure network interfaces for parallel routing
Summary(pl):	Konfiguracja interfejsów sieciowych dla równoleg³ego routingu
Summary(pt_BR):	Configura interfaces de rede para roteamento paralelo
Name:		ifenslave
Version:	0.07
Release:	3
License:	GPL
Group:		Networking
Source0:	http://www.beowulf.org/software/%{name}.c
# Source0-md5:	5472fde3f858e9c84dab9821152de55a
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

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
install -D %{name} $RPM_BUILD_ROOT/sbin/%{name}

%clean
rm -fr $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%attr(755,root,root) /sbin/ifenslave
