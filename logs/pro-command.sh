#!/bin/bash
set -euo pipefail
unset GIT_DIR GIT_OPTIONAL_LOCKS
unset DNBOX_PRESET_MEASURE_STAGE || true
cd /workspace/qa-runs/de4c13c4
export JIELI_TOOLCHAIN=/home/box/.jieli/toolchain
export AC79_SDK=/workspace/build-1214/sdk-v1.2.14
export X0X_DEV_PADCAL=0 BOARD=smk37 MODEL=pro X0X_JOBS=4 PYTHONUNBUFFERED=1
echo "START pro $(date -Iseconds)"
echo "PWD=$(pwd) HEAD=$(git rev-parse HEAD)"
echo "CMD: python3 -u tools/build.py --board smk37 --model pro --sdk /workspace/build-1214/sdk-v1.2.14 --nightly-version 0.0.26 --git-sha 94197fc839b00aff47b98ca02f3c6396e4a6eabf"
python3 -u tools/build.py --board smk37 --model pro \
  --sdk /workspace/build-1214/sdk-v1.2.14 \
  --nightly-version 0.0.26 \
  --git-sha 94197fc839b00aff47b98ca02f3c6396e4a6eabf
echo "END pro exit=$? $(date -Iseconds)"
