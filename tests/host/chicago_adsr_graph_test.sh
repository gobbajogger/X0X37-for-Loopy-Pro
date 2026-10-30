#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Chicago ENVELOPE graph sweep through the production UI. Optional SMC_UI_SHOTS saves PNGs.
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_TEST_FILE=tests/host/chicago_adsr_graph_test.c \
SMC_UI_OUT=${SMC_UI_OUT:-build/host/chicago-adsr-graph} sh tests/host/smc_ui_test.sh
