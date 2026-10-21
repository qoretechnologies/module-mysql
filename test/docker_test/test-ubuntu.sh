#!/bin/bash

set -e
set -x

ENV_FILE=/tmp/env.sh

. ${ENV_FILE}

# setup MODULE_SRC_DIR env var
cwd=`pwd`
if [ -z "${MODULE_SRC_DIR}" ]; then
    if [ -e "$cwd/src/qore-mysql-module.h" ] || [ -e "$cwd/src/mysql.cpp" ]; then
        MODULE_SRC_DIR=$cwd
    else
        MODULE_SRC_DIR=$WORKDIR/module-mysql
    fi
fi
echo "export MODULE_SRC_DIR=${MODULE_SRC_DIR}" >> ${ENV_FILE}

echo "export QORE_UID=999" >> ${ENV_FILE}
echo "export QORE_GID=999" >> ${ENV_FILE}

. ${ENV_FILE}

export MAKE_JOBS=4

# The distro ARM64 connector has incomplete coroutine asm clobbers under LTO.
if [ "$(uname -m)" = aarch64 ]; then
    connector_root=$(mktemp -d /tmp/qore-mariadb.XXXXXX)
    "${MODULE_SRC_DIR}/patches/build-mariadb.sh" "$connector_root"
    export LD_LIBRARY_PATH="$connector_root/install/lib/mariadb:${LD_LIBRARY_PATH:-}"
fi
python3 "${MODULE_SRC_DIR}/test/mariadb-async-context.py"

# build module and install
echo && echo "-- building module --"
mkdir -p ${MODULE_SRC_DIR}/build
cd ${MODULE_SRC_DIR}/build
cmake .. -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}
make -j${MAKE_JOBS}
make install

# add Qore user and group
groupadd -o -g ${QORE_GID} qore
useradd -o -m -d /home/qore -u ${QORE_UID} -g ${QORE_GID} qore

# own everything by the qore user
chown -R qore:qore ${MODULE_SRC_DIR}
if [ -n "${connector_root:-}" ]; then
    chown -R qore:qore "$connector_root"
fi

# run the tests
export QORE_MODULE_DIR=${MODULE_SRC_DIR}/qlib:${QORE_MODULE_DIR}
cd ${MODULE_SRC_DIR}

# wait for the database server to accept connections; the GitLab "services" keyword starts the
# container but does not wait for the server inside it to finish initializing, and without this
# the tests race the server and skip themselves for lack of a connection
echo && echo "-- waiting for the database server --"
db_ready=0
for i in $(seq 1 60); do
    if gosu qore:qore qore -e '%new-style
Datasource ds(ENV.QORE_DB_CONNSTR_MYSQL); ds.open(); ds.close();' >/dev/null 2>&1; then
        db_ready=1
        break
    fi
    sleep 1
done
if [ ${db_ready} -eq 0 ]; then
    echo "database server at ${QORE_DB_CONNSTR_MYSQL} did not accept connections" >&2
    exit 1
fi

for test in test/*.qtest; do
    gosu qore:qore qore -b --enable-debug "$test" -vv
done
