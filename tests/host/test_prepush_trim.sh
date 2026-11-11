#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/prepush-check.sh with a stub quickcheck.sh: a push to integration of the box repository with the same
# update hook as tools/update.hook does not run the check here ("quick check: server runs it"). All other cases run it.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
unset X0X_QC_CLIENT X0X_BOX_REPO   # the test sets them per case
D="$(cd "$(dirname "$0")/../.." && pwd)"
T=$(mktemp -d "${TMPDIR:-/tmp}/ppt.XXXXXX"); trap 'rm -rf "$T"' EXIT
mkdir -p "$T/g" && cp "$D/tools/prepush-check.sh" "$T/g/" && chmod 755 "$T/g/prepush-check.sh"
printf '#!/bin/sh\necho run >> "%s/qc.calls"\necho "QUICK PASS stub"\n' "$T" > "$T/g/quickcheck.sh"; chmod 755 "$T/g/quickcheck.sh"
G="git -c user.name=t -c user.email=t@t -c init.defaultBranch=main"
$G init -q "$T/c" && mkdir -p "$T/c/tools" && echo 'echo hook v1' > "$T/c/tools/update.hook" && echo a > "$T/c/a" && \
    $G -C "$T/c" add -A && $G -C "$T/c" commit -qm one || { echo "FAIL setup"; exit 1; }
S=$(git -C "$T/c" rev-parse HEAD); Z=0000000000000000000000000000000000000000
git init -q --bare "$T/box.git" && git init -q --bare "$T/other.git"
hooks() {  # update-content|none pre-receive(1/0)
    rm -f "$T/box.git/hooks/update" "$T/box.git/hooks/pre-receive"
    [ "$1" = none ] || { printf '%s\n' "$1" > "$T/box.git/hooks/update"; chmod 755 "$T/box.git/hooks/update"; }
    [ "$2" = 1 ] && { echo 'exit 0' > "$T/box.git/hooks/pre-receive"; chmod 755 "$T/box.git/hooks/pre-receive"; }; :; }
fails=0
chk() {  # name expect(skip|run|none) url [ref] [env...]
    n=$1; e=$2; u=$3; r=${4:-refs/heads/integration}; if [ $# -ge 4 ]; then shift 4; else shift $#; fi
    rm -f "$T/qc.calls"
    out=$(cd "$T/c" && echo "refs/heads/main $S $r $Z" | env GIT_DIR="$T/c/.git" GIT_WORK_TREE="$T/c" X0X_BOX_REPO="$T/box.git" "$@" \
        bash "$T/g/prepush-check.sh" origin "$u" 2>&1); rc=$?
    got=none; [ -f "$T/qc.calls" ] && got=run
    echo "$out" | grep -q 'quick check: server runs it' && got=skip
    [ "$got" = skip ] && ! echo "$out" | grep -q "QUICK PASS ${S%"${S#???????}"}" && got=skip-no-pass
    if [ "$got" = "$e" ] && [ $rc = 0 ]; then echo "ok   $n: $got"; else echo "FAIL $n: want $e, got $got (rc=$rc): $out"; fails=$((fails + 1)); fi
}
hooks 'echo hook v1' 1
chk "box, same update hook"            skip "$T/box.git"
chk "box as file:// URL"               skip "file://$T/box.git"
chk "box, X0X_QC_CLIENT=1"             run  "$T/box.git" refs/heads/integration X0X_QC_CLIENT=1
chk "other repository"                 run  "$T/other.git"
chk "box, not integration"             none "$T/box.git" refs/heads/lane/x
hooks 'echo hook v2' 1
chk "box, other update hook sha"       run  "$T/box.git"
hooks none 1
chk "box, no update hook"              run  "$T/box.git"
hooks 'echo hook v1' 0
chk "box, no pre-receive hook"         run  "$T/box.git"
hooks 'echo hook v1' 1; chmod 644 "$T/box.git/hooks/update"
chk "box, update hook not executable"  run  "$T/box.git"
echo "prepush trim: $fails FAIL"
[ "$fails" = 0 ]
