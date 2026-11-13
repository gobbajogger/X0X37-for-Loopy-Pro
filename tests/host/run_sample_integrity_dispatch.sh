#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual editor frames + NOR flash; focused sample integrity, not hardware proof.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
[ -f build/gen/x0x_drum_tables.h ] || sh host/build_host.sh --generate-only >/dev/null
${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-variable ${SAN_FLAGS:-} -Ibuild/gen tests/host/sample_integrity_dispatch_test.c -o build/host/sample_integrity_dispatch_test
build/host/sample_integrity_dispatch_test
