#!/bin/bash
set -euo pipefail
unset GIT_DIR GIT_OPTIONAL_LOCKS
# Explicitly do not set DNBOX_PRESET_MEASURE_STAGE (default 0).
unset DNBOX_PRESET_MEASURE_STAGE || true
cd /workspace/qa-runs/cd9a1948
export JIELI_TOOLCHAIN=/home/box/.jieli/toolchain
export AC79_SDK=/workspace/build-1214/sdk-v1.2.14
export X0X_DEV_PADCAL=0
export BOARD=smk37
export MODEL=elite
export X0X_JOBS=4
export PYTHONUNBUFFERED=1
echo "START elite $(date -Iseconds)"
echo "PWD=$(pwd)"
echo "JIELI_TOOLCHAIN=$JIELI_TOOLCHAIN"
echo "AC79_SDK=$AC79_SDK"
echo "X0X_DEV_PADCAL=$X0X_DEV_PADCAL"
echo "DNBOX_PRESET_MEASURE_STAGE=${DNBOX_PRESET_MEASURE_STAGE-<unset>}"
echo "BOARD=$BOARD MODEL=$MODEL X0X_JOBS=$X0X_JOBS"
echo "HEAD=$(git rev-parse HEAD)"
echo "TREE=$(git rev-parse HEAD^{tree})"
# Same packaging identity as the 94197fc8 baseline (FMB plan: stage-0
# image is byte-identical to a base built with the same --git-sha).
echo "CMD: python3 -u tools/build.py --board smk37 --model elite --sdk /workspace/build-1214/sdk-v1.2.14 --nightly-version 0.0.26 --git-sha 94197fc839b00aff47b98ca02f3c6396e4a6eabf"
python3 -u tools/build.py --board smk37 --model elite \
  --sdk /workspace/build-1214/sdk-v1.2.14 \
  --nightly-version 0.0.26 \
  --git-sha 94197fc839b00aff47b98ca02f3c6396e4a6eabf
echo "END elite exit=$? $(date -Iseconds)"
