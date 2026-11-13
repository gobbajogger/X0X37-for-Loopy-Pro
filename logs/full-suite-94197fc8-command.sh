#!/bin/bash
set -u
export NINEW9_DIR=/workspace/x0x37-refs/schwung/schwung-9W9
export EIGHTW8_DIR=/workspace/x0x37-refs/schwung/schwung-8W8
export SCHWUNG303=/workspace/x0x37-refs/schwung/schwung-303
export TB3PO_REF=/workspace/x0x37-refs/schwung/schwung-tb3po/src/dsp/tb3po.c
export X0X_TEST_JOBS=8
export X0X_TEST_TIME_SCALE=20
cd /workspace/qa-runs/94197fc8
echo "START $(date --iso-8601=seconds) SHA=$(git rev-parse HEAD) PWD=$PWD"
echo "NINEW9_DIR=$NINEW9_DIR"
echo "EIGHTW8_DIR=$EIGHTW8_DIR"
echo "SCHWUNG303=$SCHWUNG303"
echo "TB3PO_REF=$TB3PO_REF"
echo "X0X_TEST_JOBS=$X0X_TEST_JOBS"
sh tests/run_tests.sh
ec=$?
echo "END $(date --iso-8601=seconds) EXIT=$ec"
exit $ec
