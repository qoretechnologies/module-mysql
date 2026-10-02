# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
%global source_date_epoch_from_changelog 1
%global use_source_date_epoch_as_buildtime 1
%if v"%{rpmversion}" >= v"4.20"
%global build_mtime_policy clamp_to_source_date_epoch
%else
%global clamp_mtime_to_source_date_epoch 1
%endif
%bcond_without tests
%bcond_without docs
Name: qore-mysql-module
Version: 2.2
Release: 2%{?dist}
Summary: MySQL and MariaDB database driver for Qore
License: LGPL-2.1-or-later
URL: https://github.com/qoretechnologies/module-mysql
Source0: %{name}-%{version}.tar.xz
BuildRequires: cmake >= 3.5
BuildRequires: make
BuildRequires: gcc-c++
BuildRequires: pkgconfig(libmariadb)
BuildRequires: qore-devel >= 3.0.0~
BuildRequires: qore-rpm-macros >= 3.0.0~
%if %{with tests}
BuildRequires: python3
%if 0%{?suse_version}
BuildRequires: mariadb
BuildRequires: mariadb-client
%else
BuildRequires: mariadb-server
BuildRequires: mariadb
%endif
%endif
%if %{with docs}
BuildRequires: doxygen
%if 0%{?suse_version}
BuildRequires: util-linux
%else
BuildRequires: util-linux-core
%endif
%endif

%description
Qore DBI driver built with MariaDB Connector/C. Supports MySQL and MariaDB,
prepared statements, transactions, Unicode, columnar results and guarded
callback-driven native bulk loading. No database server is needed at runtime.

%if %{with docs}
%package doc
Summary: MySQL and MariaDB driver API reference and examples
BuildArch: noarch
%description doc
HTML API reference and examples for the Qore MySQL and MariaDB DBI driver.
%endif

%prep
%autosetup

%build
%{?set_build_flags}
. %{_rpmconfigdir}/qore/module-env.sh
qore_set_source_prefix_maps "%{qore_debug_source_dir}"
# Select the declared MariaDB provider explicitly, including on hosts that also
# have MySQL headers or libraries installed. The module then reports LGPL mode.
client_lib=$(pkg-config --variable=libdir libmariadb)/libmariadb.so
client_include=$(pkg-config --variable=includedir libmariadb)
cmake -S . -B build -G 'Unix Makefiles' \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE=-DNDEBUG \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_SKIP_RPATH=ON \
  -DCMAKE_IGNORE_PREFIX_PATH=/usr/local -DQore_DIR=%{_libdir}/cmake/Qore \
  -DQORE_EXECUTABLE=/usr/bin/qore -DQORE_QPP_EXECUTABLE=/usr/bin/qpp \
  -DQORE_GENERATE_JAVA_BINDINGS=OFF \
  -DMySQL_INCLUDE_DIR:PATH="$client_include" \
  -DMySQL_LIBS:FILEPATH="$client_lib" -DMySQL_LIBS_R:FILEPATH="$client_lib" \
  -DCMAKE_DISABLE_FIND_PACKAGE_Doxygen=%{!?with_docs:ON}%{?with_docs:OFF}
%if %{with docs}
printf '\nWARN_AS_ERROR = FAIL_ON_WARNINGS\n' >> build/Doxyfile
%endif
cmake --build build -- %{?_smp_mflags}
%if %{with docs}
cmake --build build --target docs -- %{?_smp_mflags}
%endif

%install
DESTDIR=%{buildroot} cmake --install build
chmod 755 %{buildroot}%{_libdir}/qore-modules/mysql-api-*.qmod
%if %{with docs}
install -d %{buildroot}%{_docdir}/%{name}-doc/examples
cp -a build/docs/mysql/html %{buildroot}%{_docdir}/%{name}-doc/
install -m644 test/db-test.q test/sql-stmt.q test/mysql-native-bulk-load.qtest \
  %{buildroot}%{_docdir}/%{name}-doc/examples/
hardlink -t -O %{buildroot}%{_docdir}/%{name}-doc
%endif

%check
%if %{with tests}
python3 -B -W error rpm/test_fixture.py -v
python3 -B -W error rpm/run-tests.py --build-dir build
%endif

%files
%license COPYING.LGPL COPYING.GPL
%doc README RELEASE-NOTES AUTHORS rpm/README.rst
%{_libdir}/qore-modules/mysql-api-*.qmod
%if %{with docs}
%files doc
%license COPYING.LGPL COPYING.GPL
%doc %{_docdir}/%{name}-doc/
%endif

%changelog
* Fri Oct 02 2026 David Nichols <david@qore.org> - 2.2-2
- Package the MariaDB client driver, API reference and examples.
- Test offline against a private unprivileged MariaDB over a Unix socket.

* Sat Aug 8 2026 David Nichols <david@qore.org> 2.2
- added callback-driven native bulk loading

* Tue Jan 25 2022 David Nichols <david@qore.org> 2.1
- updated version to 2.1

* Tue May 1 2018 David Nichols <david@qore.org> 2.0.2.1
- updated version to 2.0.2.1

* Sun Dec 1 2013 David Nichols <david@qore.org> 2.0.2
- updated version to 2.0.2

* Sun Nov 18 2012 David Nichols <david@qore.org> 2.0.1
- updated version to 2.0.1

* Wed Sep 26 2012 David Nichols <david@qore.org> 2.0
- updated version to 2.0

* Fri Apr 16 2010 Petr Vanek <petr.vanek@qoretechnologies.com> 1.0.8
- updated version to 1.0.8 and various typos fixed

* Thu Jun 25 2009 David Nichols <david_nichols@users.sourceforge.net>
- updated version to 1.0.6

* Thu Apr 30 2009 David Nichols <david_nichols@users.sourceforge.net>
- updated version to 1.0.5

* Tue Mar 3 2009 David Nichols <david_nichols@users.sourceforge.net>
- updated version to 1.0.4

* Mon Mar 2 2009 David Nichols <david_nichols@users.sourceforge.net>
- updated version to 1.0.3

* Sat Jan 3 2009 David Nichols <david_nichols@users.sourceforge.net>
- updated version to 1.0.2

* Tue Sep 2 2008 David Nichols <david_nichols@users.sourceforge.net>
- initial spec file for separate mysql release
