%define DIRNAME textgen
%define SPECNAME smartmet-plugin-%{DIRNAME}
Summary: SmartMet TextGen plugin
Name: %{SPECNAME}
Version: 25.3.7
Release: 1%{?dist}.fmi
License: FMI
Group: SmartMet/Plugins
URL: https://github.com/fmidev/smartmet-plugin-textgen
Source0: %{name}.tar.gz

# https://fedoraproject.org/wiki/Changes/Broken_RPATH_will_fail_rpmbuild
%global __brp_check_rpaths %{nil}

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
%if 0%{?rhel} && 0%{rhel} < 9
%define smartmet_boost boost169
%else
%define smartmet_boost boost
%endif

BuildRequires: rpm-build
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: %{smartmet_boost}-devel
BuildRequires: libconfig17-devel
BuildRequires: mysql++-devel
BuildRequires: bzip2-devel
BuildRequires: smartmet-library-calculator-devel >= 25.3.7
BuildRequires: smartmet-library-textgen-devel >= 25.3.7
BuildRequires: smartmet-library-spine-devel >= 25.2.18
BuildRequires: smartmet-engine-querydata-devel >= 25.2.18
BuildRequires: smartmet-engine-geonames-devel >= 25.2.18
BuildRequires: smartmet-engine-gis-devel >= 25.2.18
BuildRequires: smartmet-library-macgyver-devel >= 25.2.18
BuildRequires: smartmet-library-locus-devel >= 25.2.18
Requires: smartmet-library-calculator >= 25.3.7
Requires: smartmet-library-macgyver >= 25.2.18
Requires: smartmet-library-locus >= 25.2.18
Requires: smartmet-library-textgen >= 25.3.7
Requires: libconfig17
Requires: smartmet-engine-geonames >= 25.2.18
Requires: smartmet-engine-querydata >= 25.2.18
Requires: smartmet-engine-gis >= 25.2.18
Requires: smartmet-server >= 25.2.18
Requires: smartmet-library-spine >= 25.2.18
%if 0%{rhel} >= 7
Requires: %{smartmet_boost}-chrono
Requires: %{smartmet_boost}-filesystem
Requires: %{smartmet_boost}-iostreams
Requires: %{smartmet_boost}-thread
Requires: %{smartmet_boost}-timer
%endif
Provides: %{SPECNAME}
Obsoletes: smartmet-brainstorm-textgenplugin < 16.11.1
Obsoletes: smartmet-brainstorm-textgenplugin-debuginfo < 16.11.1
#TestRequires: smartmet-utils-devel >= 25.2.18
#TestRequires: smartmet-engine-gis >= 25.2.18
#TestRequires: smartmet-engine-geonames >= 25.2.18
#TestRequires: smartmet-library-spine-plugin-test >= 25.2.18
#TestRequires: smartmet-library-newbase-devel >= 25.2.18
#TestRequires: smartmet-test-data
#TestRequires: smartmet-test-db

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
* Fri Mar  7 2025 Andris Pavēnis <andris.pavenis@fmi.fi> 25.3.7-1.fmi
- Use Fmi::Exception instead of TextGen::TextGenError

* Tue Feb 18 2025 Andris Pavēnis <andris.pavenis@fmi.fi> 25.2.18-1.fmi
- Update to gdal-3.10, geos-3.13 and proj-9.5

* Fri Nov  8 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.11.8-1.fmi
- Repackage due to smartmet-library-spine ABI changes

* Tue Sep  3 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.9.3-1.fmi
- Repackage due smartmet-engine-querydata changes

* Wed Aug  7 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.8.7-1.fmi
- Update to gdal-3.8, geos-3.12, proj-94 and fmt-11

* Fri Jul 12 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.7.12-1.fmi
- Replace many boost library types with C++ standard library ones

* Thu May 16 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.5.16-1.fmi
- Clean up boost date-time uses

* Tue May  7 2024 Andris Pavēnis <andris.pavenis@fmi.fi> 24.5.7-1.fmi
- Use Date library (https://github.com/HowardHinnant/date) instead of boost date_time

* Fri Feb 23 2024 Mika Heiskanen <mika.heiskanen@fmi.fi> 24.2.23-1.fmi
- Full repackaging

* Tue Jan 30 2024 Mika Heiskanen <mika.heiskanen@fmi.fi> 24.1.30-1.fmi
- Repackaged due to newbase ABI changes

* Tue Dec  5 2023 Mika Heiskanen <mika.heiskanen@fmi.fi> - 23.12.5-1.fmi
- Repackaged due to an ABI change in SmartMetPlugin

* Fri Sep  1 2023 Mika Heiskanen <mheiskan@rhel8.dev.fmi.fi> - 23.9.1-1.fmi
- Repackaged

* Thu Aug 3 2023 Anssi Reponen <anssi.reponen@fmi.fi> - 23.8.3-1.fmi
- Fixed cache key bug (BRAINSTORM-2674)

* Fri Jul 28 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.7.28-1.fmi
- Repackage due to bulk ABI changes in macgyver/newbase/spine

* Tue Jul 11 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.7.11-1.fmi
- Use postgresql 15, gdal 3.5, geos 3.11 and proj-9.0

* Tue Jun 13 2023 Mika Heiskanen <mika.heiskanen@fmi.fi> - 23.6.13-1.fmi
- Support internal and environment variables in configuration files

* Thu Apr 27 2023 Andris Pavēnis <andris.pavenis@fmi.fi> 23.4.27-1.fmi
- Repackage due to macgyver ABI changes (AsyncTask, AsyncTaskGroup)

* Mon Dec  5 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.12.5-1.fmi
- Check HTTP request type and handle only POST and OPTIONS requests

* Wed Oct  5 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.10.5-1.fmi
- Do not use boost::noncopyable

* Thu Aug 25 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.8.25-1.fmi
- Use a generic exception handler for configuration file errors

* Thu Jul 28 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.7.28-1.fmi
- Fixed 'forecast_text_cache_size' configuration setting to work

* Wed Jul 27 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.7.27-1.fmi
- Repackaged since macgyver CacheStats ABI changed

* Tue Jun 21 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.6.21-1.fmi
- Add support for RHEL9, upgrade libpqxx to 7.7.0 (rhel8+) and fmt to 8.1.1

* Tue May 31 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.5.31-1.fmi
- Repackage due to smartmet-engine-querydata and smartmet-engine-observation ABI changes

* Tue May 24 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.24-1.fmi
- Repackaged due to NFmiArea ABI changes

* Fri May 20 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.5.20-1.fmi
- Repackaged due to ABI changes to newbase LatLon methods

* Thu Apr 28 2022 Andris Pavenis <andris.pavenis@fmi.fi> 22.4.28-1.fmi
- Repackage due to SmartMet::Spine::Reactor ABI changes

* Fri Jan 21 2022 Andris Pavēnis <andris.pavenis@fmi.fi> 22.1.21-1.fmi
- Repackage due to upgrade of packages from PGDG repo: gdal-3.4, geos-3.10, proj-8.2

* Thu Jan 13 2022 Mika Heiskanen <mika.heiskanen@fmi.fi> - 22.1.13-1.fmi
- Removed unnecessary mutex, the product cache is already thread safe

* Fri Dec 17 2021 Anssi Reponen <anssi.reponen@fmi.fi> - 21.12.17-1.fmi
- Fixed 'missing frost_forecast story'-bug. Fixed cache expiration time bug (BRAINSTORM-2222)

* Tue Dec  7 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.12.7-1.fmi
- Update to postgresql 13 and gdal 3.3

* Fri Oct 29 2021 Pertti Kinnia <pertti.kinnia@fmi.fi> - upcoming
- Added test dependency for smartmet-library-newbase-devel

* Wed Oct  6 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.10.6-1.fmi
- Used clang-tidy to modernize code

* Thu Sep 23 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.9.23-1.fmi
- Repackage to prepare for moving libconfig to different directory

* Wed Sep 22 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.9.22-1.fmi
- Protect dictionary with a mutex since the language may change during dictionary use

* Wed Sep 15 2021 Anssi Reponen <anssi.reponen@fmi.fi> - 21.9.15-3.fmi
- Added support for database_servers.postgresql_dictionary.schema configuration parameter

* Wed Sep 15 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.9.15-2.fmi
- Improved error messages

* Wed Sep 15 2021 Anssi Reponen <anssi.reponen@fmi.fi> - 21.9.15-1.fmi
- Support for PostgreSQL dictionary database added (BRAINSTORM-1707)

* Mon Sep 13 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.9.13-1.fmi
- Repackaged due to Fmi::Cache statistics fixes

* Thu Sep  9 2021 Andris Pavenis <andris.pavenis@fmi.fi> 21.9.9-1.fmi
- Repackage due to dependency change (libconfig->libconfig17)

* Mon Aug 30 2021 Anssi Reponen <anssi.reponen@fmi.fi> - 21.8.30-1.fmi
- Cache counters added (BRAINSTORM-1005)

* Tue Aug 17 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.8.17-1.fmi
- Use the new shutdown API

* Thu Jun 17 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.6.17-2.fmi
- Do not register the plugin for the server until the configuration files have been scanned once

* Thu Jun 17 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.6.17-1.fmi
- Use identical case conversions with GIS-engine

* Thu May 27 2021 Anssi Reponen <anssi.reponen@fmi.fi> - 21.5.27-1.fmi
- Added support for areasource query option in order to handle geometries with the same name (BRAINSTORM-2073): 
Optional geometry_tables.additional_tables.name configuration parameter identifies a database schema/table/field. 
If the name has been defined it must be used in areasource URL-parameter in order to use geometries defined in 
that database schema/table/field.

* Thu Feb 18 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.2.18-1.fmi
- Repackaged due to newbase ABI changes

* Mon Jan 25 2021 Anssi Reponen <anssi.reponen@fmi.fi> - 21.1.25-1.fmi
- Define explicitly the events that are subscribed from DirectoryMonitor. Related to ticket BRAINSTORM-1981.

* Thu Jan 14 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.14-1.fmi
- Repackaged smartmet to resolve debuginfo issues

* Tue Jan  5 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.5-1.fmi
- Removed incorrect dependency on observation-engine

* Tue Dec 15 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.12.15-1.fmi
- Upgrade to pgdg12

* Mon Oct 19 2020 Andris Pavenis <andris.pavenis@fmi.fi> - 20.10.19-1.fmi
- Build update: use makefile.inc from smartmet-library-macgyver

* Wed Sep 23 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.9.23-1.fmi
- Use Fmi::Exception instead of Spine::Exception

* Fri Aug 21 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.21-1.fmi
- Upgrade to fmt 6.2

* Mon Aug 10 2020 Andris Pavenis <andris.pavenis@fmi.fi> - 20.8.10-1.fmi
- Ensure that directory monitor thread is stopped when destroying plugin object

* Wed Jun 17 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.6.17-1.fmi
- Fixed shutdown to stop monitoring configuration files
- Monitor configuration files every 5 seconds instead of 10 to enable faster shutdown

* Sat Apr 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.18-1.fmi
- Upgraded to Boost 1.69

* Tue Feb 25 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.2.25-1.fmi
- Added CORS header

* Thu Jan 23 2020 Anssi Reponen <anssi.reponen@fmi.fi> - 20.1.23-2.fmi
- Fixed configuration file reading bug. (BRAINSTORM-1746)
- Broken testcases updated

* Thu Nov 14 2019 Anssi Reponen <anssi.reponen@fmi.fi> - 19.11.14-1.fmi
- Added support for bbox and wkt parameters (BRAINSTORM-1720)

* Tue Nov 5 2019 Anssi Reponen <anssi.reponen@fmi.fi> - 19.11.5-1.fmi
- Support for parameter mapping. Changed structure of configuration file for database servers (BRAINSTORM-1719)

* Thu Oct 31 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.31-1.fmi
- Rebuilt due to newbase API/ABI changes

* Wed Oct 23 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.23-1.fmi
- Added support for file dictionaries
- Refactored database details into local.conf 

* Tue Oct 22 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.22-1.fmi
- Allow running without any polygons, just geonames

* Thu Sep 26 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.26-1.fmi
- Added support for ASAN & TSAN builds

* Thu Sep 12 2019 Anssi Reponen <anssi.reponen@fmi.fi> - 19.9.12-1.fmi
- Even if forecast is found in cache, generate a new forecast if 
product configuration was modified recently (within cache interval). 
When developing and testing new product configuration, the result of
a query must reflect the changed product configuration even if query 
parameters remain unchanged. (BRAINSTORM-1676)

* Mon Sep  2 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.2-1.fmi
- Fixed a memory corruption issue

* Wed Aug 28 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.8.28-1.fmi
- Repackaged since Spine::Location ABI changed

* Thu Aug 8  2019 Anssi Reponen <anssi.reponen@fmi.fi> - 19.8.8-1.fmi
- Enable configuration parameter replacement in URL (BRAINSTORM-1653)

* Thu Feb 14 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.2.14-1.fmi
- Added client IP to exception reports

* Sun Sep 23 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.23-1.fmi
- Silenced CodeChecker warnings

* Tue Sep 18 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.18-1.fmi
- Improved (more) log messages when configuration changes are noticed (BRAINSTORM-853)

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
