Summary: A dictionary application.
Name: dict2
Version: 2.0
Release: 1
License: GPL
Group: Applications/Accessories
URL: http://students.mimuw.edu.pl/~lc235951/dict/
BuildRoot: /var/tmp/%{name}-root
Source0: %{name}-%{version}.tar.gz
Source1: dict2.desktop
Icon: dict2.xpm

%description
Dict2 is a GTK+ dictionary application intended mainly to
be used with German-English, English-German dictionaries from
www.dict.cc. However, it may display dictionaries in any language.

%prep
%setup -q
echo "$RPM_BUILD_ROOT"

%build
./configure --prefix="/usr"
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR="$RPM_BUILD_ROOT"

mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications
desktop-file-install --vendor dict \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications        \
  --add-category Utility                                \
  --add-category Application                           \
  %{SOURCE1}


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
/usr/share/applications/dict2.desktop
/usr/share/dict2/honig.txt
/usr/share/dict2/irregular_verbs_de.txt
/usr/share/dict2/dict2.glade
/usr/share/dict2/dict2.xpm
/usr/share/dict2/de.inflect
/usr/share/dict2/de.forms
/usr/share/dict2/de.stem
/usr/share/dict2/en.inflect
/usr/share/dict2/en.forms
/usr/share/dict2/en.stem
/usr/bin/dict2

%changelog
* Fri Sep 15 2006  <lc235951@localhost.localdomain> - 1.1.4-1
- Initial build.
