Native MariaDB ARM64 coroutine fix
=================================

Copyright 2026 Qore Technologies, s.r.o.

``mariadb-aarch64-clobbers.patch`` fixes the caller-saved register declarations
in MariaDB Connector/C's ARM64 coroutine assembly. Spawn omitted x8, x12, x16
and x17; continue and yield omitted x8, x16 and x17. With LTO, GCC can keep live
connection state in these registers across an inlined context switch. Ubuntu's
3.4.9 client keeps the async-context pointer in x12 and a boolean in w16, then
uses their overwritten values after returning. The result is native memory
corruption, invalid readiness masks and "Commands out of sync" errors.

The patch applies to upstream 3.4.9 and 3.4.11. It declares all of those
caller-clobbered registers; it does not disable LTO or asynchronous connections.
The unmodified source is maintained at:
https://github.com/mariadb-corporation/mariadb-connector-c

``mariadb-infile-error-bounds.patch`` also corrects the native default infile
error callback's buffer capacity handling. ``ma_strmake()`` appends a terminator
after the supplied character count, so the callback must reserve one byte and
leave a zero-capacity buffer untouched. GCC's x86 LTO build exposed this error;
the accompanying native test reproduces out-of-bounds writes for zero and short
buffers and checks twelve null-context and file-error boundary cases.

Building and testing
--------------------

Build the pinned Connector/C 3.4.11 source with the fix into an isolated prefix::

    patches/build-mariadb.sh /absolute/path/to/connector
    export LD_LIBRARY_PATH=/absolute/path/to/connector/install/lib/mariadb:$LD_LIBRARY_PATH
    python3 test/mariadb-async-context.py
    QORE_MODULE_DIR=build-debug qore -b --enable-debug test/connection-setup.qtest -vv

The native test needs no database: it checks server rejection, peer closure and
the native login timeout against owned loopback sockets. Run it in a separate
process because the unpatched library can corrupt memory. The full module suite
also checks successful login, initialization and SQL against a live database.

CI builds and uses the corrected library on ARM64. The module's normal build
does not install or patch system libraries. ARM64 deployments must apply this
patch to their connector package (or provide a corrected library using the
loader's normal search path) before using asynchronous connection setup. Merely
upgrading to unpatched 3.4.11 does not fix these missing declarations.
