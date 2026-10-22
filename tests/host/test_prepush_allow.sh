#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/pre-push.hook part 0: the remote allowlist. No push is made: the test runs the
# hook as git does (arguments: remote name and push URL, stdin: the pushed refs, here none) in a temp clone.
# Roots and box path are temp folders (X0X_PREPUSH_ROOTS, X0X_BOX_REPO). The Mac allowed path is set with
# git config x0x37.allowedRemote. Cases: tests/host/fixtures/prepush_allow/urls.txt (GitHub https, git@, ssh, the DISABLED
# push URL, a ../ escape, a path in the root that is not allowed, the box path, a symlink to it, the allowed Mac path).
# Then: the allowed Mac path with no x0x37.allowedRemote is refused.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; HK="$TOP/tools/pre-push.hook"; F="$TOP/tests/host/fixtures/prepush_allow/urls.txt"
T=$(mktemp -d "${TMPDIR:-/tmp}/ppallow.XXXXXX"); trap 'rm -rf "$T"' EXIT
T=$(cd "$T" && pwd -P)
BOX="$T/box.git"; MAC="$T/mac"
git init -q --bare "$BOX"; mkdir -p "$MAC/sub"; git init -q --bare "$MAC/x0x37"; git init -q --bare "$MAC/other.git"; git init -q --bare "$T/escape.git"
ln -s "$BOX" "$T/box-link.git"
git init -q "$T/c" && git -C "$T/c" -c user.name=t -c user.email=t@t commit -q --allow-empty -m A
git -C "$T/c" config x0x37.allowedRemote "$MAC/x0x37"
fails=0; n=0
hook() { (cd "$T/c" && X0X_PREPUSH_ROOTS="$BOX $MAC" X0X_BOX_REPO="$BOX" sh "$HK" origin "$1" < /dev/null 2>&1); }
while IFS='|' read -r want u; do
    case "$want" in "#"*|"") continue;; esac
    url=$(echo "$u" | sed -e "s|^BOX|$BOX|" -e "s|^file:BOX|file:$BOX|" -e "s|^MAC|$MAC|" -e "s|^T/|$T/|")
    n=$((n + 1)); out=$(hook "$url"); rc=$?
    if [ "$want" = OK ] && [ $rc = 0 ]; then echo "ok   allowed: $u"
    elif [ "$want" = REFUSED ] && [ $rc != 0 ] && echo "$out" | grep -q 'BLOCKED: .*Allowed:'; then echo "ok   refused: $u: $(echo "$out" | head -1 | cut -c1-100)"
    else echo "FAIL $want $u: rc=$rc $out"; fails=$((fails + 1)); fi
done < "$F"
git -C "$T/c" config --unset x0x37.allowedRemote
n=$((n + 1)); out=$(hook "$MAC/x0x37"); rc=$?
if [ $rc != 0 ] && echo "$out" | grep -q 'not the box repository or x0x37.allowedRemote'; then echo "ok   no x0x37.allowedRemote: the Mac path is refused"
else echo "FAIL no x0x37.allowedRemote: rc=$rc $out"; fails=$((fails + 1)); fi
echo "prepush allow: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
