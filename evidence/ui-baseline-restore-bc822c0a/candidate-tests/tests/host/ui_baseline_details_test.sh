#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
X0X_HOST_DEFS="${X0X_HOST_DEFS:-} -DX0X_SMK_CAPTIONS=1" NIGHT_UI_TEST_FILE=tests/host/ui_baseline_details_test.c exec sh tests/host/night_ui_test.sh "${1:-build/host/ui-baseline-details}"
