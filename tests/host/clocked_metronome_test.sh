#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
export NIGHT_UI_TEST_FILE=tests/host/clocked_metronome_test.c
exec sh tests/host/night_ui_test.sh "${1:-build/host/clocked-metronome}"
