#!/bin/sh
# Shared-source Elite DEV build. No device access occurs.
set -eu
cd /Users/mac/Documents/x0x37/smc_central_stop_repair_20261021
export CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L
export JIELI_TOOLCHAIN=/Users/mac/.jieli/toolchain AC79_SDK=/sdk
export X0X_DEV_PADCAL=1 PYTHONDONTWRITEBYTECODE=1
MODEL=elite sh build.sh --board smk37 > evidence/stop-stage2/build-elite.log 2>&1
