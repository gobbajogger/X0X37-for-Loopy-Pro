#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Compiled host paint of the production UI. Injects peer B from the header DMA poll.
set -eu
cd "$(dirname "$0")/../.."
exec env SMC_UI_TEST_FILE=tests/host/smc_lcd_paint_test.c SMC_UI_OUT="${SMC_LCD_PAINT_OUT:-build/host/smc-lcd-paint}" \
    sh tests/host/smc_ui_test.sh
