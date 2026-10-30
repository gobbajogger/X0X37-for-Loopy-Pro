#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/host
[ -f build/gen/x0x_drum_tables.h ] || sh host/build_host.sh --generate-only >/dev/null
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable \
    ${SAN_FLAGS:-} -Ibuild/gen tests/host/project_protocol_test.c -o build/host/project_protocol_test
build/host/project_protocol_test
