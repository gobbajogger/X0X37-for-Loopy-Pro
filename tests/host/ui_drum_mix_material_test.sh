#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Actual MIX909/808 renderer fixtures, state/band bounds and unchanged control maps.
set -eu
cd "$(dirname "$0")/../.."
NIGHT_UI_TEST_FILE=tests/host/ui_drum_mix_material_test.c exec sh tests/host/night_ui_test.sh "${1:-build/host/ui-drum-mix-material}"
