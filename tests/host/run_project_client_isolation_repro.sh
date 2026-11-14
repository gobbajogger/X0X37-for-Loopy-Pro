#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Open cross-client defect reproducer; deliberately outside the acceptance suite.
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/project-client-isolation}
mkdir -p "$OUT" build/gen
[ -f build/gen/x0x_drum_tables.h ] || python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable \
    ${SAN_FLAGS:-} -Ibuild/gen tests/host/project_client_dispatch.c -o "$OUT/dispatcher"
node tests/host/project_client_isolation_repro.mjs "$OUT/dispatcher" "$@"
