#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Populated actual render, compact-mark bounds, project nonmutation, no device I/O.
set -eu
cd "$(dirname "$0")/../.."
NIGHT_UI_TEST_FILE=tests/host/ui_main_material_test.c exec sh tests/host/night_ui_test.sh "${1:-build/host/ui-main-material}"
