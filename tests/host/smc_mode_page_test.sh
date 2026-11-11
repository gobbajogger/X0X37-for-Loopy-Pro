#!/bin/sh
# Every SMC preset x mixer page: fader/knob destinations, pickup, and no lost
# continuous input on a full ring. Production UI/SMC/engine; no device I/O.
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_TEST_FILE=tests/host/smc_mode_page_test.c SMC_UI_OUT=${SMC_UI_OUT:-build/host/smc-mode-page} \
    sh tests/host/smc_ui_test.sh
