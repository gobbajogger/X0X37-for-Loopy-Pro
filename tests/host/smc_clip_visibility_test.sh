#!/bin/sh
# Production UI/engine/queues: keyboard clip stages and dedicated SMC popup.
# No device I/O; use the shared integration harness instead of stale extraction.
set -eu
cd "$(dirname "$0")/../.."
export SMC_UI_TEST_FILE=tests/host/smc_clip_visibility_test.c
export SMC_UI_OUT="${SMC_CLIP_OUT:-build/host/smc-clip-visibility}"
exec sh tests/host/smc_ui_test.sh
