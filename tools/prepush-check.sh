#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Quick check for the box pre-push hook (installed as /workspace/x0x37-refs/gate/prepush-check.sh by tools/install_gate.sh).
#   prepush-check.sh <remote> <url> < <the lines of the pre-push hook: local-ref local-sha remote-ref remote-sha>
# Only a push to refs/heads/integration is checked. For each pushed commit the script runs quickcheck.sh (next to it)
# in a worktree of the pushing repository (<git common dir>/x0x37-quickcheck/wt, kept for incremental builds).
# The changed files are the remote integration tip to the pushed commit. A tree with tools/ste_strict needs an
# installed quickcheck.sh with the STE step (install_gate.sh also installs ste_check.py), else the push stops. A "QUICK FAIL" stops the push (exit code 1).
set -u
QC="$(cd "$(dirname "$0")" && pwd)/quickcheck.sh"
Z=0000000000000000000000000000000000000000
GC=$(git rev-parse --path-format=absolute --git-common-dir) || exit 0
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE   # the hook sets these; a scratch repository must not use the repository of the hook
export GIT_DIR="$GC"   # only for the git calls of this script; quickcheck.sh clears it

rc=0
while read -r lref lsha rref rsha; do
    [ -n "${lsha:-}" ] || continue
    if [ "$rref" != refs/heads/integration ]; then echo "prepush-check: $rref: no quick check (only for integration)" >&2; continue; fi
    [ "$lsha" = "$Z" ] && continue                       # a delete
    W="$GC/x0x37-quickcheck/wt"
    if [ -d "$W" ]; then env -u GIT_DIR git -C "$W" checkout -q -f --detach "$lsha" || { rc=1; continue; }
    else mkdir -p "$GC/x0x37-quickcheck"; git worktree prune; git worktree add -q --detach "$W" "$lsha" || { rc=1; continue; }; fi
    if [ -f "$W/tools/ste_strict" ] && ! grep -q '^ste_step()' "$QC"; then
        echo "prepush-check: QUICK FAIL ${lsha:0:7} STE: strict mode, but the installed $QC has no STE step (run tools/install_gate.sh)" >&2
        rc=1; continue
    fi
    BASE=""
    [ "$rsha" != "$Z" ] && git cat-file -e "$rsha^{commit}" 2>/dev/null && BASE=$rsha
    echo "prepush-check: quick check of ${lsha:0:7} for $rref (the first run in a repository also builds everything)" >&2
    OUT=$(env -u GIT_DIR X0X_QC_BASE="$BASE" "$QC" "$W" 2>&1 | tail -n1)
    echo "prepush-check: $OUT" >&2
    case "$OUT" in "QUICK PASS"*) ;; *) rc=1 ;; esac
done
exit $rc
