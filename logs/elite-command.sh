#!/bin/bash
set -u
cd /workspace/qa-runs/354564c3
export X0X_DEV_PADCAL=0
exec python3 -u tools/build.py --board smk37 --model elite --sdk /workspace/build-1214/sdk-v1.2.14 --nightly-version 0.0.26 --git-sha 94197fc839b00aff47b98ca02f3c6396e4a6eabf
