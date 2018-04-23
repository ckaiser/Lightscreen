Name: lightscreen
Version: 2.4.git7782bd5
Release: 1.6.1%{?dist}
Summary: Simple tool to automate the tedious process of saving and cataloging screenshots
URL: https://lightscreen.com.ar/
License: GPLv2

BuildRequires: qt5-qtbase-devel qt5-qtdeclarative-devel qt5-qtxmlpatterns-devel qt5-qtmultimedia-devel qt5-qtx11extras-devel
BuildRequires: make cmake dos2unix xcb-util-keysyms-devel desktop-file-utils
Requires: qt5-qtbase qt5-qtdeclarative qt5-qtxmlpatterns qt5-qtmultimedia qt5-qtx11extras xcb-util-keysyms


%define lightscreenSHA 7782bd5d68a0c14b06873d3a04929816e337c8a3
%define uglobSHA 231b10144741b29037f0128bb7a1cd7176529f74
%define singleAppSHA c6378eec45a5fdf699b4d27fb4be22a190b2a184
#patch
%define patch01SHA 6feb4628124f90f197886623c56278a1ab11ab91
%define patch01LONG 6db935d8a54f67061f0841add8f392b9-6feb4628124f90f197886623c56278a1ab11ab91
#patchend


Source0: https://github.com/ckaiser/Lightscreen/archive/%{lightscreenSHA}.zip
Source1: https://github.com/ckaiser/UGlobalHotkey/archive/%{uglobSHA}.zip
Source2: https://github.com/ckaiser/SingleApplication/archive/%{singleAppSHA}.zip
Source3: https://gist.github.com/Darksider3/6db935d8a54f67061f0841add8f392b9/archive/%{patch01SHA}.zip
Source4: https://raw.githubusercontent.com/Darksider3/lightscreen_rpm/master/lightscreen.desktop


%description
Lightscreen is a simple tool to automate the tedious process of saving and cataloging screenshots.

%prep
	cd %_sourcedir
	unzip "%{patch01SHA}.zip"
	unzip "%{lightscreenSHA}.zip"
	unzip "%{uglobSHA}.zip"
	unzip "%{singleAppSHA}.zip"
	cp -r "Lightscreen-%{lightscreenSHA}" "%{_builddir}"
	cp UGlobalHotkey-%{uglobSHA}/* "%{_builddir}"/"Lightscreen-%{lightscreenSHA}/tools/UGlobalHotkey"
	cp SingleApplication-%{singleAppSHA}/* "%{_builddir}"/"Lightscreen-%{lightscreenSHA}/tools/SingleApplication"

%build
	cp "%{_sourcedir}/%{patch01LONG}/undef_success_x11.patch" "%{_builddir}/Lightscreen-%{lightscreenSHA}/tools/"
	cp "%{_sourcedir}/lightscreen.desktop" "%{_builddir}/Lightscreen-%{lightscreenSHA}/lightscreen.desktop"
	#Patch....
	cd "%{_builddir}/Lightscreen-%{lightscreenSHA}/tools"
	unix2dos undef_success_x11.patch
	patch --ignore-whitespace --binary screenshot.cpp < undef_success_x11.patch
	cd "%{_builddir}/Lightscreen-%{lightscreenSHA}"
	#build
	qmake-qt5
	make

%install
	mkdir -p %{buildroot}/%{_bindir}
	mkdir -p %{buildroot}/usr/share/pixmaps
	mkdir -p %{buildroot}/usr/share/applications
	install -p -m 0755 "%{_builddir}/Lightscreen-%{lightscreenSHA}/lightscreen" "%{buildroot}/%{_bindir}/lightscreen"
	install -p -m 0775 "%{_builddir}/Lightscreen-%{lightscreenSHA}/images/LS.ico" "%{buildroot}/usr/share/pixmaps/lightscreen.ico"
	desktop-file-install "%{_builddir}/Lightscreen-%{lightscreenSHA}/lightscreen.desktop" "%{buildroot}/usr/share/applications/lightscreen.desktop"
	
%clean
	rm -rf %{buildroot}/*
	rm -rf %{_builddir}/*
	# clean up sourecdir afterwards(but only directories, save downloaded files)
	find %{_sourcedir} -maxdepth 1 -mindepth 1 -type d -exec rm -rf '{}' \;

%files
	/usr/bin/lightscreen
	/usr/share/pixmaps/lightscreen.ico
	/usr/share/applications/lightscreen.desktop

%changelog
* Mon Apr 23 2018 darksider3 <github@darksider3.de> - 2.4.git7782bd5-1.6.1
- Added clean-target for makefile
- Added cleanup-routines(sourcedir,builddir,buildroot)
- Added makefile for building
- Remove unneccessary(hopefully) desktop-file-install from 'required' 
  for installation of the rpm file. And fix some intendantion aswell.


* Mon Apr 23 2018 darksider3 <github@darksider3.de> - 2.4.git7782bd5-1.6
- Added Desktop File
- desktop-install-file routine
- added desktop file to files-section
* Sun Apr 22 2018 darksider3 <github@darksider3.de> - 2.4.git-1.5.1
- remove redundant cd's
* Sun Apr 22 2018 darksider3 <github@darksider3.de> - 2.4.git-1.5
- simplify trough variables!
* Sun Apr 22 2018 darksider3 <github@darksider3.de> - 2.4.git-1
- initial package release
