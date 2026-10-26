#!/bin/sh
# Validate the complete combined Elite DEV set. No device access occurs.
set -eu
cd /Users/mac/Documents/x0x37/smc_complete_stage4_20261021
export CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L
export JIELI_TOOLCHAIN=/Users/mac/.jieli/toolchain AC79_SDK=/sdk
export PYTHONDONTWRITEBYTECODE=1
python3 -B tools/dev_validate.py run \
  --commit "$1" --model elite --plan evidence/complete-stage4/dev-plan.json \
  --output evidence/complete-stage4/elite
