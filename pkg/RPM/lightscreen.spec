Name: Lightscreen
Version: 2.4.git7782bd5
Release: 1.6.2%{?dist}
Summary: Simple tool to automate the tedious process of saving and cataloging screenshots.
URL: https://lightscreen.com.ar/
VCS: https://github.com/ckaiser/Lightscreen.git
License: GPLv2

BuildRequires: qt5-qtbase-devel qt5-qtdeclarative-devel qt5-qtxmlpatterns-devel qt5-qtmultimedia-devel qt5-qtx11extras-devel
BuildRequires: make cmake dos2unix xcb-util-keysyms-devel desktop-file-utils
Requires: qt5-qtbase qt5-qtdeclarative qt5-qtxmlpatterns qt5-qtmultimedia qt5-qtx11extras xcb-util-keysyms


%global lightscreenSHA 7782bd5d68a0c14b06873d3a04929816e337c8a3
%global uglobSHA 231b10144741b29037f0128bb7a1cd7176529f74
%global singleAppSHA c6378eec45a5fdf699b4d27fb4be22a190b2a184

Source0: https://github.com/ckaiser/Lightscreen/archive/%{lightscreenSHA}.zip
Source1: https://github.com/ckaiser/UGlobalHotkey/archive/%{uglobSHA}.zip
Source2: https://github.com/ckaiser/SingleApplication/archive/%{singleAppSHA}.zip
Source3: https://raw.githubusercontent.com/Darksider3/lightscreen_rpm/master/lightscreen.desktop
#patch
%global patch01SHA 6feb4628124f90f197886623c56278a1ab11ab91
%global patch01LONG 6db935d8a54f67061f0841add8f392b9-6feb4628124f90f197886623c56278a1ab11ab91
#patchend
Patch0: https://gist.github.com/Darksider3/6db935d8a54f67061f0841add8f392b9/archive/%{patch01SHA}.zip



%description
Lightscreen is a simple tool to automate the tedious process of saving and cataloging screenshots.


%prep
	cd %_sourcedir
	%{uncompress: %{P:0}} #PATCH0
	%{uncompress: %{S:0}} #SOURCE0
	%{uncompress: %{S:1}}
	%{uncompress: %{S:2}}
	%{uncompress: %{S:3}}
	%{__cp} -r "%{name}-%{lightscreenSHA}" "%{_builddir}"
	%{__cp} UGlobalHotkey-%{uglobSHA}/* "%{_builddir}"/"%{name}-%{lightscreenSHA}/tools/UGlobalHotkey"
	%{__cp} SingleApplication-%{singleAppSHA}/* "%{_builddir}"/"%{name}-%{lightscreenSHA}/tools/SingleApplication"


%build
	%{__cp} "%{_sourcedir}/%{patch01LONG}/undef_success_x11.patch" "%{_builddir}/%{name}-%{lightscreenSHA}/tools/"
	%{__cp} "%{_sourcedir}/lightscreen.desktop" "%{_builddir}/%{name}-%{lightscreenSHA}/lightscreen.desktop"
	#Patch....
	cd "%{_builddir}/%{name}-%{lightscreenSHA}/tools"
	unix2dos undef_success_x11.patch
	%{__patch} --ignore-whitespace --binary screenshot.cpp < undef_success_x11.patch
	cd "%{_builddir}/%{name}-%{lightscreenSHA}"
	#build
	qmake-qt5
	%{__make}


%install
	%{__mkdir_p} %{buildroot}/%{_bindir}
	%{__mkdir_p} %{buildroot}/usr/share/pixmaps
	%{__mkdir_p} %{buildroot}/usr/share/applications
	#docs
	mkdir -p %{buildroot}/%{_docdir}/%{name}
	%__install -p -m 0755 "%{_builddir}/%{name}-%{lightscreenSHA}/README.md" "%{buildroot}/%{_docdir}/%{name}/README.md"
	%__install -p -m 0755 "%{_builddir}/%{name}-%{lightscreenSHA}/README.md" "%{buildroot}/%{_docdir}/%{name}/README.md"
	%__install -p -m 0755 "%{_builddir}/%{name}-%{lightscreenSHA}/LICENSE" "%{buildroot}/%{_docdir}/%{name}/LICENSE"
	chmod -x+X -R %{buildroot}/%{_docdir}/%{name}
	#/docs
	%__install -p -m 0755 "%{_builddir}/%{name}-%{lightscreenSHA}/lightscreen" "%{buildroot}/%{_bindir}/lightscreen"
	%__install -p -m 0755 "%{_builddir}/%{name}-%{lightscreenSHA}/images/LS.ico" "%{buildroot}/usr/share/pixmaps/lightscreen.ico"
	desktop-file-install "%{_builddir}/%{name}-%{lightscreenSHA}/lightscreen.desktop" "%{buildroot}/usr/share/applications/lightscreen.desktop"
	

%clean
	rm -rf %{buildroot}/*
	rm -rf %{_builddir}/*
	# clean up sourecdir afterwards(but only directories, save downloaded files)
	find %{_sourcedir} -maxdepth 1 -mindepth 1 -type d -exec rm -rf '{}' \;


%files
	%doc %{_docdir}/%{name}/README.md
	%doc %{_docdir}/%{name}/LICENSE
	%{_bindir}/lightscreen
	%{_datadir}/pixmaps/lightscreen.ico
	%{_datadir}/applications/lightscreen.desktop


%changelog
* Wed Apr 25 2018 darksider3 <github@darksider3.de> - 1.6.2
- Change all Uppercase Lightscreen to name-variable
- use uncompress-macro, not unzip
- use S/P-macro instead of SHAs
- use global instead of define
- use __cp-macro instead of cp
- use __install-macro instead of install
- __make-macro instead of make
- __mkdir_p-macro instead of mkdir-p
- __patch-macro instead of patch
- delete macro in comments
- include README.md and LICENSE!
- remove executable flag from doc-files.
- reindent changelog


* Mon Apr 23 2018 darksider3 <github@darksider3.de> - 1.6.1
- Added clean-target for makefile
- Added cleanup-routines(sourcedir,builddir,buildroot)
- Added makefile for building
- Remove unneccessary(hopefully) desktop-file-install from 'required' 
  for installation of the rpm file. And fix some intendantion aswell.

* Mon Apr 23 2018 darksider3 <github@darksider3.de> - 1.6
- Added Desktop File
- desktop-install-file routine
- added desktop file to files-section

* Sun Apr 22 2018 darksider3 <github@darksider3.de> - 1.5.1
- remove redundant cd's

* Sun Apr 22 2018 darksider3 <github@darksider3.de> - 1.5
- simplify trough variables!

* Sun Apr 22 2018 darksider3 <github@darksider3.de> - 1
- initial package release
