#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
for test in lcd_bounded_test lcd_bounded_scenarios_test; do
  ${CC:-cc} -O2 -std=c11 -Wall -Wextra -Werror -I. "tests/host/$test.c" -o "build/host/$test"
  "build/host/$test"
done
