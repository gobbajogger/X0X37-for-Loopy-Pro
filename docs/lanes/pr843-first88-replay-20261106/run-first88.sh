#!/bin/bash
set -uo pipefail
cd /workspace/pr843-first88/source
export PATH=/workspace/pr843-first88/toolchain/root/usr/lib/llvm-19/bin:$PATH
export LD_LIBRARY_PATH=/workspace/pr843-first88/toolchain/root/usr/lib/x86_64-linux-gnu
export CC=clang CXX=clang++
export TB3PO_REF=/workspace/pr843-first88/inputs/charlesvestal-schwung-tb3po-f49d81d/src/dsp/tb3po.c
export X0X_GEN_CACHE=/workspace/pr843-first88/cache
export X0X_DEV_PADCAL=1 X0X_STRICT_PREFLIGHT=1 X0X_TEST_JOBS=8 X0X_TEST_TIME_SCALE=20
export X0X_TESTS="$(cat /workspace/pr843-first88/evidence/selectors.txt)"
mkdir -p "$X0X_GEN_CACHE"
date -u +%FT%TZ > /workspace/pr843-first88/evidence/started-at.txt
sh tests/run_tests.sh 2>&1 | tee /workspace/pr843-first88/evidence/stdout.log
rc=${PIPESTATUS[0]}
printf '%s\n' "$rc" > /workspace/pr843-first88/evidence/exit-code.txt
date -u +%FT%TZ > /workspace/pr843-first88/evidence/finished-at.txt
exit "$rc"
