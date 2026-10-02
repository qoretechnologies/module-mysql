#!/usr/bin/python3
# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
"""Exercise packaged MySQL modules against a private Unix-socket MariaDB."""
import argparse
from contextlib import contextmanager
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import re
import select
import shutil
import signal
import subprocess
import tempfile
import time

def await_startup(stream, timeout=20):
    deadline = time.monotonic() + timeout
    output = bytearray()
    while not re.search(rb'ready for connections\.', output):
        ready, _, _ = select.select([stream], [], [], max(0, deadline - time.monotonic()))
        if not ready:
            raise RuntimeError('MariaDB startup deadline exceeded: ' + output.decode(errors='replace'))
        data = os.read(stream.fileno(), 4096)
        if not data:
            raise RuntimeError('MariaDB server exited before startup: ' + output.decode(errors='replace'))
        output.extend(data)
    return output.decode(errors='replace')


@contextmanager
def running_server(command, env, logs=None):
    server = subprocess.Popen(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              start_new_session=True)
    # Drain the pipe throughout the suites, so negative authentication tests
    # cannot block mariadbd by filling its logging pipe.
    with ThreadPoolExecutor(max_workers=1) as reader:
        output = None
        try:
            startup = await_startup(server.stdout)
            if logs is not None:
                logs.append(startup)
            print(startup, end='', flush=True)
            output = reader.submit(server.stdout.read)
            yield server
        finally:
            try:
                os.killpg(server.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                server.wait(timeout=20)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(server.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                server.wait()
                raise
            finally:
                try:
                    text = output.result() if output else server.stdout.read()
                finally:
                    server.stdout.close()
                decoded = text.decode(errors='replace')
                if logs is not None:
                    logs.append(decoded)
                print(decoded, end='', flush=True)


def validate_diagnostics(text, authentication_test=False):
    """Accept only the two explicitly approved external-server diagnostics."""
    mac = denied = 0
    for line in text.splitlines():
        if not re.search(r'\[(?:Warning|ERROR)\]|^WARNING:', line, re.I):
            continue
        if re.fullmatch(r"\d{4}-\d\d-\d\d +\d+:\d\d:\d\d +0 \[Warning\] failed to retrieve the MAC address", line):
            mac += 1
        elif authentication_test and re.fullmatch(
                r"\d{4}-\d\d-\d\d +\d+:\d\d:\d\d +\d+ \[Warning\] Access denied for user 'root'@'localhost' \(using password: YES\)", line):
            denied += 1
        else:
            raise RuntimeError('Unexpected MariaDB diagnostic: ' + line)
    if mac > 1 or denied != int(authentication_test):
        raise RuntimeError(f'Unexpected diagnostic counts: MAC={mac}, authentication={denied}')


def module_path(build, env):
    paths = subprocess.check_output(['/usr/bin/qore', '--module-path'], env=env, text=True).strip().split(':')
    directories = [build.resolve()] if build else [Path(path) for path in paths]
    modules = [module for directory in directories for module in directory.glob('mysql-api-*.qmod')]
    if len(modules) != 1:
        raise RuntimeError('Expected exactly one MySQL native module: ' + repr(modules))
    env.update(QORE_MODULE_DIR=':'.join(dict.fromkeys([str(modules[0].parent), *paths])),
               QORE_MODULE_DIR_ONLY='1')
    return modules[0]


def run(build=None, compiler=False):
    if os.getuid() == 0:
        raise RuntimeError('MariaDB package tests must run unprivileged')
    source = Path(__file__).resolve().parents[1]
    env = os.environ.copy()
    for key in list(env):
        if key.startswith(('MYSQL_', 'MARIADB_', 'QORE_DB_CONNSTR_')) or key in (
                'QORE_MODULE_DIR', 'QORE_MODULE_DIR_ONLY', 'QORE_INCLUDE_DIR', 'LD_LIBRARY_PATH', 'LD_PRELOAD'):
            env.pop(key)
    env.update(LC_ALL='C.UTF-8', TZ='UTC')
    module = module_path(build, env)
    qore = ['/usr/bin/qore', '-b', '--enable-debug', '--lgpl', '-l', str(module)]
    server = shutil.which('mariadbd', path='/usr/sbin:/usr/bin')
    if not server:
        raise RuntimeError('mariadbd is required for package qualification')
    with tempfile.TemporaryDirectory(prefix='qore-mysql-rpm-') as directory:
        root = Path(directory)
        # The directory is mode 0700. No TCP listener or host database is used.
        env.update(MYSQL_UNIX_PORT=str(root / 'server.sock'), QORE_DB_CONNSTR_MYSQL='mysql:root/@qoretest')
        # Bound this small fixture's caches and connections below OBS's 1024-FD limit.
        # Keep resource warnings fatal rather than widening the diagnostic exception.
        local_settings = ['--wsrep-node-address=127.0.0.1', '--wsrep-node-incoming-address=127.0.0.1',
                          '--open-files-limit=512', '--table-open-cache=64', '--table-open-cache-instances=1',
                          '--max-connections=64']
        initialized = subprocess.run(['mariadb-install-db', '--no-defaults', '--datadir=' + str(root / 'data'),
                        '--auth-root-authentication-method=normal', '--skip-test-db', '--force', *local_settings],
                       env=env, cwd=root, check=True, timeout=120, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        print(initialized.stdout, end='', flush=True)
        validate_diagnostics(initialized.stdout)
        tests = root / 'test'; tests.mkdir()
        suites = sorted((source / 'test').glob('*.qtest'))
        if len(suites) != 3:
            raise RuntimeError('Review the MySQL suite inventory before qualification')
        for suite in suites:
            (tests / suite.name).write_text(re.sub(r'^%prepend-module-path .*\n', '', suite.read_text(), flags=re.M))
        command = [server, '--no-defaults', '--datadir=' + str(root / 'data'),
                   '--socket=' + env['MYSQL_UNIX_PORT'], '--pid-file=' + str(root / 'server.pid'),
                   '--skip-networking', '--local-infile=1', '--innodb-buffer-pool-size=64M', *local_settings]
        server_logs = []
        with running_server(command, env, server_logs):
            subprocess.run(['mariadb', '--no-defaults', '--socket=' + env['MYSQL_UNIX_PORT'], '--user=root',
                            '-e', 'CREATE DATABASE qoretest CHARACTER SET utf8mb4'], env=env, check=True, timeout=30)
            subprocess.run([*qore, '-e', 'Datasource db(ENV.QORE_DB_CONNSTR_MYSQL); db.open(); db.close();'],
                           env=env, cwd=root, check=True, timeout=30)
            for suite in suites:
                subprocess.run([*qore, str(tests / suite.name), '-v'], env=env, cwd=root, check=True, timeout=300)
            if compiler:
                # Reuse the packaged compiler example without its server wrapper.
                text = (source / 'debian/tests/compiler').read_text().split("<<'EOF'\n", 1)[1].split('\nEOF', 1)[0]
                (root / 'mysql-smoke.q').write_text(text + '\n')
                subprocess.run(['/usr/bin/qcc', '-o', str(root / 'mysql-smoke'), str(root / 'mysql-smoke.q')],
                               env=env, cwd=root, check=True, timeout=120)
                subprocess.run([str(root / 'mysql-smoke')], env=env, cwd=root, check=True, timeout=30)
        validate_diagnostics(''.join(server_logs), authentication_test=True)
        print('All three MySQL suites passed against the private MariaDB.', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--build-dir', type=Path)
    mode.add_argument('--installed', action='store_true')
    parser.add_argument('--compiler', action='store_true')
    arguments = parser.parse_args()
    run(arguments.build_dir, arguments.compiler)
