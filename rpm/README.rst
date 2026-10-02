RPM packaging
=============

Copyright 2026 Qore Technologies, s.r.o.

The portable recipe uses MariaDB Connector/C and packages the LGPL driver and
a separate HTML reference with examples. The MariaDB
server is a build-test dependency only. The pinned source archive excludes
``m4/acx_pthread.m4``, matching Debian's source licensing audit; CMake does not use it.

From qore-packaging, prepare and build the source::

    python3 tools/packaging.py prepare --repo ../module-mysql --ref COMMIT \
      --name qore-mysql-module --version 2.2 --spec qore-mysql-module.spec \
      --exclude m4/acx_pthread.m4 --output work/mysql-source
    python3 tools/build-local.py --source work/mysql-source \
      --image TARGET_SDK_IMAGE --output results/mysql-build --jobs 4

The fixture creates a private database directory and Unix socket, with no TCP
listener or changes to system databases. It reads MariaDB's startup event,
drains logs during tests and reaps the server on success and failure. Connection
preflight ensures an unavailable database fails qualification instead of skipping.
All three suites run with debugging enabled and Qore signal handling disabled.

For installed-package qualification, install the runtime RPM, Python and MariaDB
in a minimal image without qore-devel or a compiler, and run as a regular user::

    python3 -B -W error rpm/run-tests.py --installed

Run the same command with ``--compiler`` in an SDK image to compile and execute
the parameterized database example from ``debian/tests/compiler``. The test
fixture copies suites outside the checkout and explicitly loads the selected
native module to prevent a development or installed copy from masking it.

This DBI-only module exports no Qore classes/functions, so it has no separate
compiler metadata files; compiled applications use the core DBI API and load the
driver at runtime. The compiler example verifies that path.

The standalone fixture explicitly supplies local WSREP addresses and initializes
grants without a DNS preflight because the build has no network interfaces or DNS.
The server still has networking disabled; no Galera provider is loaded.

Approved diagnostic exception (2026-10-02)
-----------------------------------------

Qualification permits at most one ``failed to retrieve the MAC address`` warning
per initialization/server process in the network-disabled container. MariaDB's
``calculate_server_uid()`` unconditionally queries a hardware address and assigns
``unknown`` if no interface has one. This only affects the server identifier;
all 34 driver cases / 203 assertions pass. The fixture keeps the log visible.
Source: https://github.com/MariaDB/server/blob/11.8/sql/mysqld.cc

The error-information suite intentionally tries an incorrect root password and
asserts error 1045 / SQLSTATE 28000. Exactly one corresponding ``Access denied``
server entry is required for a complete suite run. No other warning/error is
accepted. Unit tests reject altered messages, unexpected accounts, duplicate or
missing authentication entries, duplicate MAC warnings and all error severities.
These two fixture diagnostics were explicitly approved by the maintainer; no
warning is disabled in the driver, its compiler, documentation or Qore tests.

The private database sets bounded connection/table cache and its instance counts and a 512-file
request, so qualification also works with OBS's 1024-file process limit.
Resource-limit warnings remain errors; they are not part of the diagnostic
exception. Qualification runs under an explicit 1024-file hard limit.
