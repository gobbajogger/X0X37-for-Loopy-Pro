#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host audio and lifecycle regression; synthetic samples only.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
${CC:-cc} -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -Wno-error=attributes ${SAN_FLAGS:-} tests/host/break_performance_test.c -lm -o build/host/break_performance_test
build/host/break_performance_test
