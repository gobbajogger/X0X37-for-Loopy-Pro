#!/bin/bash
set -uo pipefail
unset GIT_DIR GIT_OPTIONAL_LOCKS GIT_WORK_TREE GIT_INDEX_FILE
cd /workspace/qa-runs/5849cb11
export NINEW9_DIR=/workspace/x0x37-refs/schwung/schwung-9W9
export EIGHTW8_DIR=/workspace/x0x37-refs/schwung/schwung-8W8
export SCHWUNG303=/workspace/x0x37-refs/schwung/schwung-303
export TB3PO_REF=/workspace/x0x37-refs/schwung/schwung-tb3po/src/dsp/tb3po.c
export PYTHONUNBUFFERED=1
echo "START full-suite $(date -Iseconds)"
echo "PWD=$(pwd) HEAD=$(git rev-parse HEAD)"
echo "NINEW9_DIR=$NINEW9_DIR"
echo "EIGHTW8_DIR=$EIGHTW8_DIR"
echo "SCHWUNG303=$SCHWUNG303"
echo "TB3PO_REF=$TB3PO_REF"
echo "X0X_TEST_JOBS=${X0X_TEST_JOBS:-8 (default)}"
echo "CMD: sh tests/run_tests.sh"
sh tests/run_tests.sh
rc=$?
echo "END full-suite exit=$rc $(date -Iseconds)"
exit $rc
