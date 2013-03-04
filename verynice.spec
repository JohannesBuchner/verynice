Summary: A Dynamic Process Re-nicer
Name: verynice
Version: VERSION
Release: 1
Copyright: GPL
Group: System Environment/Daemons
Source: http://www.tam.cornell.edu/~sdh4/verynice/verynice-VERSION.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot

%description
VeryNice is a tool for dynamically adjusting the nice-level of processes
under UNIX-like operating systems. It can also be used to kill off 
runaway processes and increase the priority of multimedia applications, 
while properly handling both batch computation jobs and interactive 
applications with long periods of high CPU usage. 

%prep
%setup -n verynice
mv verynice.init verynice.init.orig
sed 's/PREFIX=\/usr\/local/PREFIX=\/usr/' <verynice.init.orig >verynice.init

%build
make PREFIX=/usr RPM_OPT_FLAGS="$RPM_OPT_FLAGS"
strip verynice

%install

rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/etc/rc.d
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
mkdir -p $RPM_BUILD_ROOT/usr

make PREFIX=/usr RPM_BUILD_ROOT=$RPM_BUILD_ROOT install
install -m 755 verynice.init $RPM_BUILD_ROOT/etc/rc.d/init.d

%clean
rm -rf $RPM_BUILD_ROOT


%post
/sbin/chkconfig --add verynice.init

%preun
if [ "$1" = 0 ] ; then
  /sbin/chkconfig --del verynice.init
fi


%files
%defattr(-,root,root)
/usr/sbin/verynice
%config /etc/rc.d/init.d/verynice.init
%config /etc/verynice.conf
/usr/doc/verynice-VERSION

