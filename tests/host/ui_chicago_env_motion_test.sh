#!/bin/sh
# Chicago ENV glyph follows the live filter EG (production UI + engine, host).
set -eu
cd "$(dirname "$0")/../.."
SMC_UI_OUT=${SMC_UI_OUT:-build/host/chicago-env-motion} SMC_UI_TEST_FILE=tests/host/ui_chicago_env_motion_test.c \
    sh tests/host/smc_ui_test.sh
