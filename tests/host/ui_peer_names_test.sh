#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
NIGHT_UI_TEST_FILE=tests/host/ui_peer_names_test.c exec sh tests/host/night_ui_test.sh "${1:-build/host/ui-peer-names}"
