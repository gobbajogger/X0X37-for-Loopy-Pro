#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-013 Stage 4: confirm tip fxbus.c / stereo_test.c blobs match Stage-2 repair.
set -euo pipefail
# Worker images mount host-owned worktrees; allow git without rewriting global config.
git() { command git -c safe.directory="$PWD" -c safe.directory='*' "$@"; }
if [[ -d .git || -f .git ]] && [[ -f firmware/src/dsp/fxbus.c ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
want_fxbus=49a89483e1c777a46758ed370973afb85f15274d
want_test=c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39
got_fxbus=$(git rev-parse HEAD:firmware/src/dsp/fxbus.c)
got_test=$(git rev-parse HEAD:tests/host/stereo_test.c)
echo "HEAD=$(git rev-parse HEAD)"
echo "fxbus.c=$got_fxbus"
echo "stereo_test.c=$got_test"
test "$got_fxbus" = "$want_fxbus"
test "$got_test" = "$want_test"
grep -E -q 'bpm >= 20\.0f' firmware/src/dsp/fxbus.c
grep -E -q '20\.0|20 BPM|bpm.*20' tests/host/stereo_test.c
echo "PASS bf013 tip blobs"
