#!/bin/sh
# Stage 2 affected DEV builds. No device, controller, or release work.
set -eu
cd /Users/mac/Documents/x0x37/smc_mapping_fix_20261021
export CC=cc CXX=c++ X0X_HOST_DEFS=-D_POSIX_C_SOURCE=200809L
export JIELI_TOOLCHAIN=/Users/mac/.jieli/toolchain AC79_SDK=/sdk
export X0X_DEV_PADCAL=1 PYTHONDONTWRITEBYTECODE=1
for model in elite pro; do
    MODEL="$model" sh build.sh --board smk37 > "evidence/mapping-stage2/build-$model.log" 2>&1 || {
        tail -60 "evidence/mapping-stage2/build-$model.log"
        exit 1
    }
    tail -10 "evidence/mapping-stage2/build-$model.log"
done
