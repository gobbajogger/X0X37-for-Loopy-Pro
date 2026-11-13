#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
qb_outer_tmp=$(mktemp -d)
trap 'rm -rf "$qb_outer_tmp"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -Wall -Wextra tests/host/outer_pack_test.c -o "$qb_outer_tmp/test"
"$qb_outer_tmp/test"
