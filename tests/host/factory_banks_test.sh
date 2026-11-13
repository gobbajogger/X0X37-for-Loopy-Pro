#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host proof for built-in Chicago/Miami factory banks + user import isolation.
set -e
cd "$(dirname "$0")/../.."
OUT=build/factory_banks_host
mkdir -p "$OUT"
CC="${CC:-cc}"
$CC -std=c99 -O2 -Wall -Wextra -o "$OUT/factory_banks_test" \
  tests/host/factory_banks_test.c
"$OUT/factory_banks_test"
