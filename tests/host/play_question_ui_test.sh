#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Real UI/SMC path via the existing production host fixture.
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_OUT=${SMC_UI_OUT:-build/host/play-question-ui} \
SMC_UI_TEST_FILE=tests/host/play_question_ui_test.c sh tests/host/smc_ui_test.sh
