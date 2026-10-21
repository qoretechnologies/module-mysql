#!/bin/sh
# Copyright 2026 Qore Technologies, s.r.o.
# Build the corrected native connector without changing the system installation.
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 ABSOLUTE_BUILD_ROOT" >&2
    exit 2
fi
case "$1" in
    /*) connector_root=$1 ;;
    *) echo "build root must be absolute" >&2; exit 2 ;;
esac
patch_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
connector_commit=be67a4fc1e0493913732df90e562f122bff9dfe3
connector_sha256=b37f7d99d3753ec12f54b19c86a004e500471560795d3546fc9f7718641a604c
mkdir -p "$connector_root"
curl -fL "https://codeload.github.com/mariadb-corporation/mariadb-connector-c/tar.gz/$connector_commit" \
    -o "$connector_root/source.tar.gz"
printf '%s  %s\n' "$connector_sha256" "$connector_root/source.tar.gz" | sha256sum -c -
tar -xzf "$connector_root/source.tar.gz" -C "$connector_root"
connector_src="$connector_root/mariadb-connector-c-$connector_commit"
patch -d "$connector_src" -p1 < "$patch_dir/mariadb-aarch64-clobbers.patch"
patch -d "$connector_src" -p1 < "$patch_dir/mariadb-infile-error-bounds.patch"
# LTO is intentional: it exposed the missing clobbers in Ubuntu's ARM64 client.
cmake -S "$connector_src" -B "$connector_src/build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$connector_root/install" -DCMAKE_C_FLAGS="-std=gnu11 -flto" \
    -DWITH_UNIT_TESTS=OFF
cmake --build "$connector_src/build" -j "${MAKE_JOBS:-4}"
python3 "$patch_dir/../test/mariadb-async-context.py" "$connector_src/build/libmariadb/libmariadb.so"
cmake --install "$connector_src/build"
cc -std=c11 -Wall -Wextra -Werror "$patch_dir/../test/mariadb-infile-error.c" \
    -I"$connector_root/install/include/mariadb" -L"$connector_root/install/lib/mariadb" \
    -Wl,-rpath,"$connector_root/install/lib/mariadb" -lmariadb -o "$connector_root/infile-error-test"
"$connector_root/infile-error-test"
