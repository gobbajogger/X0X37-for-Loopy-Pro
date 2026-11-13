#!/bin/bash
set -euo pipefail
unset GIT_DIR GIT_OPTIONAL_LOCKS
unset DNBOX_PRESET_MEASURE_STAGE || true
cd /workspace/qa-runs/5849cb11
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
echo "BOARD=$BOARD MODEL=$MODEL X0X_JOBS=$X0X_JOBS"
echo "HEAD=$(git rev-parse HEAD)"
echo "TREE=$(git rev-parse HEAD^{tree})"
echo "CMD: python3 -u tools/build.py --board smk37 --model elite --sdk /workspace/build-1214/sdk-v1.2.14 --nightly-version 0.0.26 --git-sha 5849cb114b20212c22d75b28839d021696fd4d68"
python3 -u tools/build.py --board smk37 --model elite \
  --sdk /workspace/build-1214/sdk-v1.2.14 \
  --nightly-version 0.0.26 \
  --git-sha 5849cb114b20212c22d75b28839d021696fd4d68
echo "END elite exit=$? $(date -Iseconds)"
