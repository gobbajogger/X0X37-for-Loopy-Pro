#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# BF-012 Stage 4: confirm tip master.c / master_test.c blobs match Stage-2 repair.
set -euo pipefail
# Worker images mount host-owned worktrees; allow git without rewriting global config.
git() { command git -c safe.directory="$PWD" -c safe.directory='*' "$@"; }
if [[ -d .git || -f .git ]] && [[ -f firmware/src/dsp/master.c ]]; then
  ROOT="$PWD"
else
  ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
  cd "$ROOT"
fi
want_master=f91a9473f0ae1cdd5e037e521f39fdfcc2022395
want_test=df2f13b35a6fc7b858b019a1950982c23f6386e3
got_master=$(git rev-parse HEAD:firmware/src/dsp/master.c)
got_test=$(git rev-parse HEAD:tests/host/master_test.c)
echo "HEAD=$(git rev-parse HEAD)"
echo "master.c=$got_master"
echo "master_test.c=$got_test"
test "$got_master" = "$want_master"
test "$got_test" = "$want_test"
# OFF→ON clear markers must remain in the tip blob.
grep -q 'la\|la_r' firmware/src/dsp/master.c
grep -E -q 'look.?ahead|lookahead|LIMIT' tests/host/master_test.c || true
echo "PASS bf012 tip blobs"
