#!/bin/sh
# Production peer-name aggregation and native-font UI render. No device I/O.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_peer_names_test.c
export SMC_UI_OUT="${SMC_PEER_NAMES_OUT:-build/host/smc-peer-names}"
mkdir -p "$SMC_UI_OUT"
awk '/^unsigned plat_ble_peer_names\(/ || /^const char \*plat_ble_name\(void\)/{on=1} on{print} on && /^}/{on=0}' firmware/src/ble.c > "$SMC_UI_OUT/ble_peer_names.inc"
test -s "$SMC_UI_OUT/ble_peer_names.inc"
exec sh tests/host/smc_ui_test.sh
