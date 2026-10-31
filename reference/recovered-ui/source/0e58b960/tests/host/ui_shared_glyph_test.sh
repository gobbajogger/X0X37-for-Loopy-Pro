#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Shared parameter schematics: every mapped page, endpoints, bounds and maps.
set -eu
cd "$(dirname "$0")/../.."
NIGHT_UI_TEST_FILE=tests/host/ui_shared_glyph_test.c exec sh tests/host/night_ui_test.sh "${1:-build/host/ui-shared-glyph}"
