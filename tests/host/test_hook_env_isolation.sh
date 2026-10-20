#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# A git hook runs with GIT_DIR, GIT_WORK_TREE and GIT_INDEX_FILE set (pre-push), and an update hook also with the
# quarantine variables. The tests that make a scratch repository must not use that repository.
# This test sets these variables to a decoy repository, runs each scratch-repository test (QC_ISO_TESTS, default:
# all tests/host/test_*.sh with "git init" or "git clone", but not this test) and checks that the HEAD, the refs,
# the index, the log, the config and the objects of the decoy are the same after the tests.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
D="$(cd "$(dirname "$0")/../.." && pwd)"
T=$(mktemp -d "${TMPDIR:-/tmp}/hei.XXXXXX"); trap 'rm -rf "$T"' EXIT
Y="$T/decoy"; mkdir -p "$Y" && cd "$Y" && git init -q . && echo keep > keep.txt && git add keep.txt && \
    git -c user.name=d -c user.email=d@d commit -qm decoy && cd "$D" || { echo "FAIL setup"; exit 1; }
snap() {  # the state of the decoy repository
    { git -C "$Y" rev-parse HEAD; git -C "$Y" symbolic-ref -q HEAD; git -C "$Y" for-each-ref; git -C "$Y" log --format='%H %an %s'
      sha1sum < "$Y/.git/index"; sha1sum < "$Y/.git/config"; find "$Y/.git/objects" -type f | sort; ls "$Y"; } 2>&1; }
B=$(snap)
fails=0; n=0
LIST=${QC_ISO_TESTS:-$(grep -lE 'git (init|clone)' tests/host/test_*.sh | grep -v test_hook_env_isolation.sh)}
for t in $LIST; do
    n=$((n + 1))
    GIT_DIR="$Y/.git" GIT_WORK_TREE="$Y" GIT_INDEX_FILE="$Y/.git/index" GIT_PREFIX= sh "$t" > "$T/out" 2>&1; rc=$?
    A=$(snap)
    if [ "$A" = "$B" ]; then echo "ok   $t (rc=$rc): the decoy repository did not change"
    else echo "FAIL $t (rc=$rc): the decoy repository changed"; fails=$((fails + 1)); fi
done
echo "decoy: HEAD $(git -C "$Y" rev-parse --short HEAD), $(git -C "$Y" rev-list --count HEAD) commit(s)"
echo "hook env isolation: $((n - fails)) of $n ok"
[ "$fails" = 0 ] && [ "$n" -gt 0 ]
