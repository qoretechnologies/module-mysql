#!/usr/bin/env python3
# Copyright 2026 Qore Technologies, s.r.o.
"""Exercise the native async connector against deterministic loopback peers.

No Qore or database server is required. Pass an optional libmariadb filename.
Run in a separate process: an unpatched ARM64 client can corrupt native memory.
"""

import ctypes as C
import ctypes.util
import select
import socket
import sys
import threading
import time
import unittest


LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 else ctypes.util.find_library("mariadb")
if not LIBRARY:
    raise RuntimeError("libmariadb is required")
LIB = C.CDLL(LIBRARY)


def api(name, result, *args):
    function = getattr(LIB, name)
    function.restype = result
    function.argtypes = args
    return function


init = api("mysql_init", C.c_void_p, C.c_void_p)
options = api("mysql_options", C.c_int, C.c_void_p, C.c_int, C.c_void_p)
close = api("mysql_close", None, C.c_void_p)
error = api("mysql_errno", C.c_uint, C.c_void_p)
start = api("mysql_real_connect_start", C.c_int, C.POINTER(C.c_void_p), C.c_void_p,
            C.c_char_p, C.c_char_p, C.c_char_p, C.c_char_p, C.c_uint, C.c_char_p, C.c_ulong)
resume = api("mysql_real_connect_cont", C.c_int, C.POINTER(C.c_void_p), C.c_void_p, C.c_int)
get_socket = api("mysql_get_socket", C.c_int, C.c_void_p)
get_timeout = api("mysql_get_timeout_value_ms", C.c_uint, C.c_void_p)

# Public mysql.h enum values; MYSQL_OPT_NONBLOCK is specific to MariaDB.
CONNECT_TIMEOUT, NONBLOCK = 0, 6000
READ, WRITE, EXCEPT, TIMEOUT = 1, 2, 4, 8


class AsyncContextTest(unittest.TestCase):
    def connect(self, mode):
        failures = []
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            listener.listen(1)
            listener.settimeout(5)

            def serve():
                try:
                    with listener.accept()[0] as peer:
                        peer.settimeout(5)
                        if mode == "reject":
                            payload = b"\xff\x15\x04#28000fixture rejection"
                            peer.sendall(len(payload).to_bytes(3, "little") + b"\0" + payload)
                        elif mode == "timeout":
                            self.assertEqual(peer.recv(1), b"")
                except BaseException as exception:
                    failures.append(exception)

            worker = threading.Thread(target=serve)
            db = init(None)
            self.assertTrue(db)
            worker.start()
            try:
                seconds = C.c_uint(1)
                self.assertEqual(options(db, CONNECT_TIMEOUT, C.byref(seconds)), 0)
                self.assertEqual(options(db, NONBLOCK, None), 0)
                result = C.c_void_p()
                status = start(C.byref(result), db, b"127.0.0.1", b"fixture", b"", None,
                               listener.getsockname()[1], None, 0)
                deadline = time.monotonic() + 5
                while status:
                    self.assertEqual(status & ~(READ | WRITE | EXCEPT | TIMEOUT), 0,
                                     f"native coroutine returned invalid wait mask: {status:#x}")
                    remaining = deadline - time.monotonic()
                    self.assertGreater(remaining, 0, "native operation exceeded test deadline")
                    fd = get_socket(db)
                    self.assertGreaterEqual(fd, 0)
                    timeout = min(remaining, get_timeout(db) / 1000) if status & TIMEOUT else remaining
                    readable, writable, exceptional = select.select(
                        [fd] if status & READ else [], [fd] if status & WRITE else [],
                        [fd] if status & EXCEPT else [], timeout)
                    ready = (READ if readable else 0) | (WRITE if writable else 0) | (EXCEPT if exceptional else 0)
                    status = resume(C.byref(result), db, ready or TIMEOUT)
                self.assertFalse(result.value)
                self.assertEqual(error(db), 1045 if mode == "reject" else 2013)
            finally:
                close(db)
                worker.join()
            if failures:
                raise failures[0]

    def test_server_rejection(self):
        self.connect("reject")

    def test_peer_close(self):
        self.connect("close")

    def test_native_timeout(self):
        self.connect("timeout")


if __name__ == "__main__":
    unittest.main(verbosity=2)
