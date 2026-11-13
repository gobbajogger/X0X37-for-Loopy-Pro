#!/bin/bash
set -euo pipefail
unset GIT_DIR GIT_OPTIONAL_LOCKS
cd /workspace/build-1214/modwheel-measure
export JIELI_TOOLCHAIN=/home/box/.jieli/toolchain
export AC79_SDK=/workspace/build-1214/sdk-v1.2.14
export X0X_DEV_PADCAL=0
export PYTHONUNBUFFERED=1
python3 -u tools/build.py --board smk37 --model elite --sdk /workspace/build-1214/sdk-v1.2.14 \
  --nightly-version 0.0.26 --git-sha 77a7dd3bf5c78c54b45395d461b5c2d75e896be1
