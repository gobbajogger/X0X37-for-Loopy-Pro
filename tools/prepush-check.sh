#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Quick check for the box pre-push hook (installed as /workspace/x0x37-refs/gate/prepush-check.sh by tools/install_gate.sh).
#   prepush-check.sh <remote> <url> < <the lines of the pre-push hook: local-ref local-sha remote-ref remote-sha>
# Only a push to refs/heads/integration is checked. For each pushed commit the script runs quickcheck.sh (next to it)
# in a worktree of the pushing repository (<git common dir>/x0x37-quickcheck/wt, kept for incremental builds).
# The changed files are the remote integration tip to the pushed commit. A QUICK FAIL stops the push (exit code 1).
# Trim: a push to integration of the box repository (X0X_BOX_REPO [/workspace/x0x37.git]) does not run the check
# here when that repository has an executable hooks/pre-receive and hooks/update, and the sha256 of hooks/update is
# the same as tools/update.hook of the pushed commit or of the remote integration tip. The update hook then runs the
# same quick check (and stops the push on a fail). The script writes "QUICK PASS <sha> deferred, quick check: server
# runs it" for the pre-push hook. All other pushes run the check here (fail closed). X0X_QC_CLIENT=1: always run here.
set -u
QC="$(cd "$(dirname "$0")" && pwd)/quickcheck.sh"
Z=0000000000000000000000000000000000000000
GC=$(git rev-parse --path-format=absolute --git-common-dir) || exit 0
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE   # the hook sets these; a scratch repository must not use the repository of the hook
export GIT_DIR="$GC"   # only for the git calls of this script; quickcheck.sh clears it
URL=${2:-}
rpath() { p=${1#file://}; [ -n "$p" ] && [ -d "$p" ] && (cd "$p" && pwd -P); }
server_runs_qc() {      # pushed-sha remote-sha: 0 when the update hook of the box repository runs the same quick check
    [ "${X0X_QC_CLIENT:-0}" = 1 ] && return 1
    u=$(rpath "$URL") || return 1; b=$(rpath "${X0X_BOX_REPO:-/workspace/x0x37.git}") || return 1
    [ "$u" = "$b" ] && [ -x "$b/hooks/update" ] && [ -f "$b/hooks/update" ] && [ -x "$b/hooks/pre-receive" ] || return 1
    SH=$(sha256sum < "$b/hooks/update" | cut -d' ' -f1)
    for c in "$1" "$2"; do
        [ "$c" != "$Z" ] && git cat-file -e "$c:tools/update.hook" 2>/dev/null || continue
        [ "$(git cat-file blob "$c:tools/update.hook" | sha256sum | cut -d' ' -f1)" = "$SH" ] && SC=$c && return 0
    done
    return 1
}

rc=0
while read -r lref lsha rref rsha; do
    [ -n "${lsha:-}" ] || continue
    if [ "$rref" != refs/heads/integration ]; then echo "prepush-check: $rref: no quick check (only for integration)" >&2; continue; fi
    [ "$lsha" = "$Z" ] && continue                       # a delete
    if server_runs_qc "$lsha" "$rsha"; then
        echo "prepush-check: QUICK PASS ${lsha:0:7} deferred, quick check: server runs it (box update hook ${SH:0:12} = tools/update.hook of ${SC:0:7})" >&2
        continue
    fi
    W="$GC/x0x37-quickcheck/wt"
    if [ -d "$W" ]; then env -u GIT_DIR git -C "$W" checkout -q -f --detach "$lsha" || { rc=1; continue; }
    else mkdir -p "$GC/x0x37-quickcheck"; git worktree prune; git worktree add -q --detach "$W" "$lsha" || { rc=1; continue; }; fi
    BASE=""
    [ "$rsha" != "$Z" ] && git cat-file -e "$rsha^{commit}" 2>/dev/null && BASE=$rsha
    echo "prepush-check: quick check of ${lsha:0:7} for $rref (the first run in a repository also builds everything)" >&2
    OUT=$(env -u GIT_DIR X0X_QC_BASE="$BASE" "$QC" "$W" 2>&1 | tail -n1)
    echo "prepush-check: $OUT" >&2
    case "$OUT" in "QUICK PASS"*) ;; *) rc=1 ;; esac
done
exit $rc
