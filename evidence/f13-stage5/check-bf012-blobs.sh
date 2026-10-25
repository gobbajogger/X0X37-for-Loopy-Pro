#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-012 Stage 5 land tip: master_test blob matches Stage-2; master.c carries OFF→ON clear
# on current nightly (blob differs from f91a9473… because nightly already shared RATIO_V).
set -euo pipefail
git() { command git -c safe.directory="$PWD" -c safe.directory='*' "$@"; }
if [[ -d .git || -f .git ]] && [[ -f firmware/src/dsp/master.c ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
want_master=7f30fbc65374cdf5db0bbc8c0641aaab96dd776d
want_test=df2f13b35a6fc7b858b019a1950982c23f6386e3
got_master=$(git rev-parse HEAD:firmware/src/dsp/master.c)
got_test=$(git rev-parse HEAD:tests/host/master_test.c)
echo "HEAD=$(git rev-parse HEAD)"
echo "master.c=$got_master"
echo "master_test.c=$got_test"
test "$got_master" = "$want_master"
test "$got_test" = "$want_test"
grep -q 'MST_LIMIT && v > 0 && !m->pot\[MST_LIMIT\]' firmware/src/dsp/master.c
grep -q 'la_r\[j\] = 0.0f' firmware/src/dsp/master.c
grep -q 'stale look-ahead audio' tests/host/master_test.c
echo "PASS bf012 land tip blobs"
