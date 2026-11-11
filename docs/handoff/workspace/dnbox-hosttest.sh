#!/bin/bash
# Needs clang: box gcc 14 rejects fastmath.h (-Werror=attributes). clang = zig cc wrapper in ~/cc-bin (venv ~/zigvenv, pip ziglang).
# Default X0X_ORIGINAL_SYNTH=0: the =1 build needs private headers (experiments/*_private.h) that are deliberately not on the box/repo.
# Usage: dnbox-hosttest.sh <label> <base-ref> [pr-number]
# Test-merges PR head (refs/pull/N/head) onto base in a scratch worktree (never pushes/commits to a branch),
# runs tests/run_tests.sh, writes /workspace/hosttest-results/<label>.{log,summary}.
set -u
REPO=/workspace/gh-x0x37; LABEL=$1; BASE=$2; PR=${3:-}
OUT=/workspace/hosttest-results; mkdir -p "$OUT" /workspace/x0x37-refs
WT=/workspace/ht-$LABEL
cd "$REPO" || exit 2
git config --local --get credential.helper >/dev/null || git config --local credential.helper '!f(){ echo username=x-access-token; echo "password=$GH_TOKEN"; };f'
git fetch -q origin "+$BASE:refs/ht/base-$LABEL" || exit 2
BASESHA=$(git rev-parse refs/ht/base-$LABEL)
git worktree remove --force "$WT" 2>/dev/null; rm -rf "$WT"
git worktree add -q --detach "$WT" "$BASESHA" || exit 2
HEAD=none; CLEAN=n/a
if [ -n "$PR" ]; then
  git fetch -q origin "+pull/$PR/head:refs/ht/pr-$PR" || exit 2
  HEAD=$(git rev-parse refs/ht/pr-$PR)
  # merge without commit, no author needed
  if git -C "$WT" merge --no-commit --no-ff -q "$HEAD" >/dev/null 2>&1; then CLEAN=yes
  else CLEAN=no; echo "label=$LABEL head=$HEAD clean=no conflicts=$(git -C "$WT" diff --name-only --diff-filter=U | tr '\n' ' ')" > "$OUT/$LABEL.summary"; exit 1; fi
fi
cd "$WT"
export PATH=$HOME/cc-bin:$PATH CC=clang CXX=clang++ X0X_ORIGINAL_SYNTH=${X0X_ORIGINAL_SYNTH:-0} X0X_TEST_JOBS=${X0X_TEST_JOBS:-3}
export SCHWUNG303=/workspace/schwung-303 EIGHTW8_DIR=/workspace/schwung-8W8 NINEW9_DIR=/workspace/schwung-9W9 TB3PO_REF=/workspace/schwung-tb3po
timeout 3600 tests/run_tests.sh > "$OUT/$LABEL.log" 2>&1; RC=$?
P=$(grep -cE '^ *(PASS|ok)\b' "$OUT/$LABEL.log"); F=$(grep -cE '^ *FAIL\b' "$OUT/$LABEL.log"); N=$(grep -cE '^ *not-run\b' "$OUT/$LABEL.log")
FIRST=$(grep -m1 -E '^ *FAIL\b' "$OUT/$LABEL.log")
echo "label=$LABEL base=$BASESHA head=$HEAD clean=$CLEAN rc=$RC pass=$P fail=$F notrun=$N first_fail=$FIRST" > "$OUT/$LABEL.summary"
grep -E "^  FAIL " "$OUT/$LABEL.log" | sed "s/ (see.*//" | sort -u > "$OUT/$LABEL.fails"
cat "$OUT/$LABEL.summary"
