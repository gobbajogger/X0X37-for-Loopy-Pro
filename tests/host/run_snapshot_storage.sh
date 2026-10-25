#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/host
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function ${SAN_FLAGS:-} tests/host/snapshot_storage_test.c -o build/host/snapshot_storage_test
build/host/snapshot_storage_test
