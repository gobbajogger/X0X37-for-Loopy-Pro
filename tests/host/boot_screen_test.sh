#!/bin/sh
# Host-only simulated NOR records and static picture format; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/host
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Wno-unused-function -Ifirmware/src tests/boot_screen_test.c -o build/host/boot-screen-test
exec build/host/boot-screen-test
