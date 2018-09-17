%define DIRNAME textgen
%define SPECNAME smartmet-plugin-%{DIRNAME}
Summary: SmartMet TextGen plugin
Name: %{SPECNAME}
Version: 18.9.17
Release: 1%{?dist}.fmi
License: FMI
Group: SmartMet/Plugins
URL: https://github.com/fmidev/smartmet-plugin-textgen
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: boost-devel
BuildRequires: libconfig-devel
BuildRequires: mysql++-devel
BuildRequires: bzip2-devel
BuildRequires: smartmet-library-calculator-devel >= 18.9.16
BuildRequires: smartmet-library-textgen-devel >= 18.9.16
BuildRequires: smartmet-library-spine-devel >= 18.9.13
BuildRequires: smartmet-engine-observation-devel >= 18.9.3
BuildRequires: smartmet-engine-querydata-devel >= 18.9.11
BuildRequires: smartmet-engine-geonames-devel >= 18.8.30
BuildRequires: smartmet-engine-gis-devel >= 18.9.17
BuildRequires: smartmet-library-macgyver-devel >= 18.9.5
BuildRequires: smartmet-library-locus-devel >= 18.8.21
Requires: smartmet-library-calculator >= 18.9.16
Requires: smartmet-library-macgyver >= 18.9.5
Requires: smartmet-library-locus >= 18.8.21
Requires: smartmet-library-textgen >= 18.9.16
Requires: libconfig
Requires: smartmet-engine-observation >= 18.9.3
Requires: smartmet-engine-geonames >= 18.8.30
Requires: smartmet-engine-querydata >= 18.9.11
Requires: smartmet-engine-gis >= 18.9.17
Requires: smartmet-server >= 18.9.12
Requires: smartmet-library-spine >= 18.9.13
%if 0%{rhel} >= 7
Requires: boost-chrono
Requires: boost-date-time
Requires: boost-filesystem
Requires: boost-iostreams
Requires: boost-thread
Requires: boost-timer
%endif
Provides: %{SPECNAME}
Obsoletes: smartmet-brainstorm-textgenplugin < 16.11.1
Obsoletes: smartmet-brainstorm-textgenplugin-debuginfo < 16.11.1

%description
SmartMet TextGen plugin

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{SPECNAME}
 
%build -q -n %{SPECNAME}
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_datadir}/smartmet/plugins/%{DIRNAME}.so

%changelog
* Mon Sep 17 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.17-1.fmi
- Improved log messages when configuration changes are noticed (BRAINSTORM-853)

* Fri Sep 14 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.14-1.fmi
- debug option now prints execution log even if there weren't any errors

* Thu Sep 13 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.13-1.fmi
- Added printlog option, with printlog=1 you get the execution log to stdout
- Added debug option, with debug=1 you get the log to html response

* Wed Sep 12 2018  Anssi Reponen <anssi.reponen@fmi.fi> - 18.9.12-1.fmi
- Configuration files re-read automatically after changes (BRAINSTORM-853)

* Sat Sep  8 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.8-1.fmi
- Silenced CodeChecker warnings

* Mon Aug 13 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.13-1.fmi
- Repackaged since Spine::Location size changed

* Wed Jul 25 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.7.25-1.fmi
- Prefer nullptr over NULL

* Mon Jul 23 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.7.23-1.fmi
- Silenced CodeChecker warnings

* Sat Apr  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.7-1.fmi
- Upgrade to boost 1.66

* Tue Mar 20 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.3.20-1.fmi
- Full recompile of all server plugins

* Thu Nov 30 2017 Anssi Reponen <anssi.reponen@fmi.fi> - 17.11.30-1.fmi
- Start using GisEngine instead of PostGISDataSource class (BRAINSTORM-722)

* Mon Aug 28 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

* Tue Apr 25 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.4.25-1.fmi
- Product paths are now relative to the configuration file itself

* Wed Mar 15 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.3.15-1.fmi
- Recompiled since Spine::Exception changed

* Thu Jan 12 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.12-1.fmi
- Recompiled with smartmet-library-calculator

* Wed Jan  4 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.4-1.fmi
- Changed to use renamed SmartMet base libraries

* Wed Nov 30 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.30-1.fmi
- No installation for configuration

* Tue Nov  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.11.1-1.fmi
- Namespace and directory name changed

* Tue Sep 20 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.9.20-1.fmi
- Replaced direct Locus-library function calls with GeoEngine function calls

* Tue Sep  6 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.9.6-1.fmi
- New exception handler

* Tue Aug 30 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.8.30-1.fmi
- Base class API change

* Mon Aug 15 2016 Markku Koskela <markku.koskela@fmi.fi> - 16.8.15-1.fmi
- The init(),shutdown() and requestHandler() methods are now protected methods
- The requestHandler() method is called from the callRequestHandler() method

* Wed Jun 29 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.29-1.fmi
- QEngine API changed

* Tue Jun 14 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.14-1.fmi
- Full recompile

* Thu Jun  2 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.2-1.fmi
- Full recompile

* Wed Jun  1 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.6.1-1.fmi
- Added graceful shutdown

* Mon Jan 18 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.18-1.fmi
- newbase API changed, full recompile

* Wed Nov 18 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.11.18-1.fmi
- SmartMetPlugin now receives a const HTTP Request

* Mon Oct 26 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.10.26-1.fmi
- Added proper debuginfo packaging

* Wed Aug 26 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.26-1.fmi
- Recompiled with latest newbase with faster parameter changing

* Mon Aug 24 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.24-1.fmi
- Recompiled due to Convenience.h API changes

* Tue Aug 18 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.18-1.fmi
- Recompile forced by brainstorm API changes

* Mon Aug 17 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.17-1.fmi
- Use -fno-omit-frame-pointer to improve perf use

* Fri Aug 14 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.8.14-1.fmi
- Recompiled due to string formatting changes

* Mon May 25 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.5.25-1.fmi
- Rebuilt against new QEngine

* Wed Apr 29 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.29-1.fmi
- Removed optionengine dependency

* Wed Apr 15 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.15-1.fmi
- Added server thread pool deduction
- newbase API changed

* Tue Apr 14 2015 Santeri Oksman <santeri.oksman@fmi.fi> - 15.4.14-1.fmi
- Rebuild

* Thu Apr  9 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.9-1.fmi
- newbase API changed

* Wed Apr  8 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.8-1.fmi
- Using dynamic linking for smartmet libraries

* Wed Feb 25 2015 Tuomo Lauri <tuomo.lauri@fmi.fi> - 15.2.25-1.fmi
- Rebuilt against the new textgen library

* Tue Feb 24 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.2.24-1.fmi
- Recompiled due to newbase linkage changes

* Wed Jan  7 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.1.7-1.fmi
- Recompiled due to obsengine API changes

* Thu Dec 18 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.12.18-1.fmi
- Recompiled due to spine API changes

* Tue Dec  9 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.12.9-1.fmi
- Defined areas for "iltaan_asti" and "ilta_ja_huominen" - products

* Mon Oct 13 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.10.13-1.fmi
- Fixed bug in road weather product configuration

* Mon Oct  6 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.10.6-1.fmi
- Migrated from startre to popper-gis

* Tue Sep  9 2014 Anssi Reponen <anssi.reponen@fmi.fi> - 14.9.9-1.fmi
- Fixed return code and error messages in HTTP response (JIRA: BRAINSTORM-343)

* Mon Sep  8 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.9.8-1.fmi
- Recompiled due to geoengine API changes

* Mon May 26 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.5.26-1.fmi
- Fixed encoding issues in forecaster-entry injection

* Fri May 23 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.5.24-1.fmi
- Fixed unintentional header in road weather product

* Fri May 23 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.5.23-2.fmi
- Added the forecaster-entry to road weather product

* Thu May 22 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.5.22-2.fmi
- Adjusted roadweather product and added coastal text generation
- Added ELY areas to recognized areas

* Thu May 22 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.5.22-1.fmi
- Added pop-and-rainfall story to roadweather product

* Wed May 21 2014 Tuomo Lauri <tuomo.lauri@fmi.fi> - 14.5.21-1.fmi
- Added road areas to road weather configuration

* Mon May 19 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.19-1.fmi
- Added tiesaa product for road weather forecasts

* Thu May 15 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.15-1.fmi
- Convert Scandinavian letters in location names to ASCII when accessing configuration files

* Wed May 14 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.14-2.fmi
- Use shared macgyver and locus libraries

* Wed May 14 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.14-1.fmi
- Location names are now always in lower case in configuration files

* Tue May 13 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.13-1.fmi
- Added Vapo product

* Fri May  9 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.9-1.fmi
- Recompiled to get the latest locus library

* Tue May  6 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.6-1.fmi
- qengine API changed

* Mon Apr 28 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.4.28-1.fmi
- Full recompile due to large changes in spine etc APIs
- Support for geoid parameter added (Thu Apr 3 2014 Reponen)

* Wed Apr  9 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.4.9-1.el6.fmi
- Database is now in mysli.fmi.fi

* Tue Nov  5 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.11.5-1.el6.fmi
- The plugin is now initialized in a separate thread so as not to delay the main server

* Tue Sep 18 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.9.18-1.el6.fmi
- Recompile due to changes in macgyver

* Tue Aug  7 2012 lauri    <tuomo.lauri@fmi.fi>    - 12.8.7-1.el6.fmi
- Location API changed

* Tue Jul 31 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.31-1.el6.fmi
- GeoNames API changed

* Thu Jul 26 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.26-1.el6.fmi
- GeoNames API changed

* Tue Jul 24 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.24-1.el6.fmi
- GeoNames API changed

* Mon Jul 23 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.23-1.el6.fmi
- Added ApparentTemperature

* Thu Jul 19 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.19-1.el6.fmi
- GeoNames API changed

* Tue Jul 10 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.10-1.el6.fmi
- Recompiled since Table changed

* Thu Jul  5 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.7.5-1.el6.fmi.fi
- Upgrade to boost 1.50

* Fri Jun  8 2012 oksman <santeri.oksman@fmi.fi> - 12.6.8-1.el6.fmi.fi
- Recompile forced due fixes in JsonFormatter.

* Tue Apr 10 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.4.10-1.el5.fmi
- Fixed WXML output not to print "nan" as a value field

* Wed Apr  4 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.4.6-1.el6.fmi
- full recompile due to common lib change

* Wed Apr  4 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.4.4-1.el6.fmi
- qengine API changed

* Mon Apr  2 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.4.2-1.el6.fmi
- macgyver change forced recompile

* Sat Mar 31 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.3.31-1.el5.fmi
- Upgrade to boost 1.49

* Tue Dec 27 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.12.27-2.el5.fmi
- Table class changed, recompile forced

* Wed Dec 21 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.12.12-1.el6.fmi
- RHEL6 release

* Tue Aug 16 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.8.16-1.el5.fmi
- Upgrade to boost 1.47

* Thu Mar 24 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.3.24-1.el5.fmi
- Upgrade to boost 1.46

* Mon Feb 14 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.2.14-1.el5.fmi
- Recompiled to use OptionEngine

* Tue Jan 18 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.1.18-1.el5.fmi
- Refactored query string parsing

* Fri Nov 26 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.11.26-1.el5.fmi
- Added cache headers

* Thu Oct 28 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.10.28-1.el5.fmi
- Recompile due to external API changes

* Wed Oct 27 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.10.27-1.el5.fmi
- Fixed DST problems again

* Wed Oct 20 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.10.20-1.el5.fmi
- Fixed DST problems

* Wed Sep 22 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.9.22-1.el5.fmi
- Improved error messages to the terminal

* Tue Sep 14 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.9.14-1.el5.fmi
- Upgrade to boost 1.44

* Mon Aug  9 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.8.9-1.el5.fmi
- Updated GeoEngine

* Wed Jul 21 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.7.21-3.el5.fmi
- xml.tag is now supported in textgen.conf
- Added WindChill parameter

* Tue Jul  6 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.7.6-1.el5.fmi
- Wxml formatting now includes location coordinates

* Mon Jul  5 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.7.5-1.el5.fmi
- Recompile brainstorm due to newbase hessaa bugfix

* Thu May 27 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.5.27-1.el5.fmi
- Zero size responses are now reported to the terminal

* Wed May  5 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.5.5-1.el5.fmi
- Fixed DST problem with Cairo

* Thu Apr 29 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.4.29-1.el5.fmi
- Moved formatters to common library

* Tue Apr 20 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.4.20-1.el5.fmi
- Fixed timesteps to work correctly when time or hour option is used

* Tue Apr 13 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.4.13-1.el5.fmi
- Fixed JSON mimetype

* Sat Mar 27 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.3.27-1.el5.fmi
- Fixed another DST problem when constructing midnight times

* Wed Mar 24 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.3.24-1.el5.fmi
- Fixed DST problem when the hour option is used

* Tue Mar 23 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.3.23-1.el5.fmi
- Added meta parameter Cloudiness8th

* Tue Feb 16 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.2.16-1.el5.fmi
- Enabled "model=default" selection

* Fri Jan 15 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.1.15-1.el5.fmi
- Upgrade to boost 1.41

* Tue Jan 12 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.1.12-1.el5.fmi
- Bug fix to timestep=graph when starttime is not given

* Mon Jan 11 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.1.11-1.el5.fmi
- Added support for timestep=graph

* Wed Dec 30 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.12.30-1.el5.fmi
- Fixed bug in WindCompass8

* Tue Dec 29 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.12.29-1.el5.fmi
- Added WindCompass8, WindCompass16 and WindCompass32

* Fri Dec 11 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.12.11-1.el5.fmi
- Added startstep=N option for skipping forward in the forecast data

* Wed Dec  9 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.12.9-1.el5.fmi
- Added iso2 parameter

* Tue Dec  8 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.12.8-1.el5.fmi
- Fixed json formatting of arrays

* Thu Nov 26 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.11.26-2.el5.fmi
- Set SigWaveHeight normal precision to 1 decimal

* Thu Nov 26 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.11.26-1.el5.fmi
- Added latlon and lonlat parameters to textgen

* Mon Nov 23 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.11.23-1.el5.fmi
- Added conversion of weekdays to UTF8

* Thu Oct 29 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.10.29-1.el5.fmi
- Recompile with new qengine

* Wed Jul 22 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.7.22-1.el5.fmi
- QEngine API changed forced recompile

* Tue Jul 14 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.7.14-1.el5.fmi
- Upgrade to boost 1.39

* Fri Mar 27 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.3.27-1.el5.fmi
- hour option did not work correctly over DST changes

* Wed Mar 25 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.3.25-1.el5.fmi
- Fixed timestep, previously only multiples of 60 worked
- Fixed epochtime calculation

* Thu Feb  5 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.2.5-1.el5.fmi
- Fixed wxml formatting not to crash if no places are given

* Mon Jan 26 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.1.26-2.el5.fmi
- Added starttime=data and endtime=data options

* Mon Jan 26 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.1.26-1.el5.fmi
- Location name is now searched for latlon coordinates
- Fixed WXML formatting

* Fri Dec 19 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.19-1.el5.fmi
- Newbase parameter names are now case independent

* Tue Dec 16 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.16-4.el5.fmi
- Added maxdistance parameter

* Tue Dec 16 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.16-3.el5.fmi
- No error is generated if data does not cover given places in keyword mode

* Fri Dec 12 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.12-2.el5.fmi
- WXML timeformat is now forced to be XML

* Fri Dec 12 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.12-1.el5.fmi
- Fixed PHP and serial formatting
- Added origintime parameter
- Added origintime to WXML
- Improved error handling

* Wed Dec 10 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.10-1.el5.fmi
- Updates to level handling
- New fminames, geoengine, qengine

* Mon Dec  8 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.8-2.el5.fmi
- Fixed xml header
- Fixed textgen mimetypes

* Thu Dec  4 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.4-1.el5.fmi
- Fixed bug in WxmlFormatter

* Wed Dec  3 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.12.3-1.el5.fmi
- Fixed country searches not to prioritize Finland first

* Thu Oct 23 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.10.23-1.el5.fmi
- Linked with updated macgyver library

* Mon Oct  6 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.10.6-1.el5.fmi
- First release
