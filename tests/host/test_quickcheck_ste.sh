#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The STE step of tools/quickcheck.sh (X0X_QUICK_ONLY=ste: no build, no tests) on a temp git repository.
#   A commit with a semicolon, a contraction and a passive in its message, and a .md file with a long sentence:
#   warn mode -> "QUICK PASS" and "STE WARN" lines. Strict mode (X0X_STE_STRICT=1, or the file tools/ste_strict
#   in the tree) -> "QUICK FAIL ... STE: ...". A clean commit -> no STE line.
set -u
unset X0X_STE_STRICT X0X_QC_BASE
Q="$(cd "$(dirname "$0")/../.." && pwd)/tools/quickcheck.sh"
T=$(mktemp -d "${TMPDIR:-/tmp}/qcste.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
ok() { n=$((n + 1)); echo "ok   $1"; }
bad() { n=$((n + 1)); fails=$((fails + 1)); echo "FAIL $1"; printf '%s\n' "$2" | sed 's/^/     /'; }
R="$T/repo"; mkdir -p "$R" && cd "$R" && git init -q . && git checkout -q -b integration || { echo "FAIL setup"; exit 1; }
gc() { git -c user.name=t -c user.email=t@t commit -q "$@"; }
echo base > a.txt && git add . && gc -m "Add the first file." -m "The file has one line."
git checkout -q -b bad integration
printf '# Notes\n\nThe quick check examines the changed files and the commit messages and then it writes one line with the result and the time and the reason for each failure that it finds.\n' > NOTES.md
git add . && gc -m "Fix the build; it's done" -m "The file was changed by the tool."
BAD=$(git rev-parse HEAD)
mkdir -p tools && touch tools/ste_strict && git add . && gc -m "Add the strict file." -m "The quick check uses strict mode now."
STRICTF=$(git rev-parse HEAD)
git checkout -q -b clean integration
echo two > b.txt && git add . && gc -m "Add the second file." -m "The file has one line."
CLEAN=$(git rev-parse HEAD)
cd - >/dev/null
touch "$T/pre-push"
qc() { X0X_QUICK_ONLY=ste X0X_REPO="$R" X0X_QC_WORK="$T/qc" X0X_PREPUSH_HOOK="$T/pre-push" bash "$Q" "$@" 2>&1; }
out=$(qc "$BAD"); rc=$?
if [ $rc = 0 ] && echo "$out" | grep -q '^QUICK PASS ' && echo "$out" | grep -q '^STE WARN [0-9]* issues' &&
   echo "$out" | grep -q 'semicolon' && echo "$out" | grep -q 'contraction' && echo "$out" | grep -q 'passive' &&
   echo "$out" | grep -q 'NOTES.md: \[long_desc'; then ok "warn mode: QUICK PASS with STE WARN"; echo "$out" | sed "s/^/     /"
else bad "warn mode (rc=$rc)" "$out"; fi
out=$(X0X_STE_STRICT=1 qc "$BAD"); rc=$?
if [ $rc = 1 ] && echo "$out" | grep -q '^QUICK FAIL .* STE: '; then ok "strict (X0X_STE_STRICT=1): $(echo "$out" | grep '^QUICK')"
else bad "strict env (rc=$rc)" "$out"; fi
out=$(qc "$STRICTF"); rc=$?
if [ $rc = 1 ] && echo "$out" | grep -q '^QUICK FAIL .* STE: '; then ok "strict (tools/ste_strict in the tree): $(echo "$out" | grep '^QUICK')"
else bad "strict file (rc=$rc)" "$out"; fi
out=$(X0X_STE_STRICT=1 qc "$CLEAN"); rc=$?
if [ $rc = 0 ] && echo "$out" | grep -q '^QUICK PASS ' && ! echo "$out" | grep -q 'STE WARN'; then ok "clean commit, strict: no STE line ($(echo "$out" | grep '^QUICK'))"
else bad "clean commit (rc=$rc)" "$out"; fi
echo "quickcheck STE: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
