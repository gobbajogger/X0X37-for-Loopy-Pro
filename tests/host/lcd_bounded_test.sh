#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
${CC:-cc} -O2 -std=c11 -Wall -Wextra -Werror -I. \
  tests/host/lcd_bounded_test.c -o build/host/lcd_bounded_test
build/host/lcd_bounded_test
${CC:-cc} -O2 -std=c11 -Wall -Wextra -Werror -I. \
  tests/host/lcd_trace_stats_test.c -o build/host/lcd_trace_stats_test
build/host/lcd_trace_stats_test
${CC:-cc} -O2 -std=c11 -Wall -Wextra -Werror -I. \
  tests/host/lcd_bounded_scenarios_test.c -o build/host/lcd_bounded_scenarios_test
build/host/lcd_bounded_scenarios_test
