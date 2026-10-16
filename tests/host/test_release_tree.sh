#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/release_tree_check.sh (check 7, rule 1): a release tree has no refs/jieli or refs/oscuro path and no refs-oscuro commit.
# A temp repository: base B. For each line of tests/host/fixtures/release_tree/paths.txt, a commit on B adds that path.
# A branch refs-oscuro (commit O on B): O itself and a merge of O are refused; B is OK. X0X_OSCURO_COMMITS names O too.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; CK="$TOP/tools/release_tree_check.sh"; F="$TOP/tests/host/fixtures/release_tree/paths.txt"
T=$(mktemp -d "${TMPDIR:-/tmp}/reltree.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
expect() {  # expect <OK|REFUSED> <name> <commit> [env...]
    want=$1; name=$2; c=$3; shift 3; n=$((n + 1))
    out=$(env "$@" sh "$CK" --repo "$T/r/.git" "$c" 2>&1); rc=$?
    case "$want" in OK) wrc=0;; *) wrc=1;; esac
    case "$out" in "TREE $want "*) [ "$rc" = "$wrc" ] && { echo "ok   $name: $out"; return; };; esac
    echo "FAIL $name: rc=$rc out=$out"; fails=$((fails + 1))
}
gc() { git -C "$T/r" -c user.name=t -c user.email=t@t commit -q "$@"; }
git init -q "$T/r" && echo a > "$T/r/a.c" && git -C "$T/r" add . && gc -m B && B=$(git -C "$T/r" rev-parse HEAD) || { echo "FAIL setup"; exit 1; }
expect OK "clean base" "$B" X0X_OSCURO_COMMITS=
while IFS='|' read -r want p; do
    case "$want" in "#"*|"") continue;; esac
    git -C "$T/r" checkout -q -B t "$B"; mkdir -p "$T/r/$(dirname "$p")"; echo x > "$T/r/$p"; git -C "$T/r" add -A; gc -m "add $p"
    expect "$want" "path $p" "$(git -C "$T/r" rev-parse HEAD)" X0X_OSCURO_COMMITS=
done < "$F"
git -C "$T/r" checkout -q -b refs-oscuro "$B"; echo o > "$T/r/o.c"; git -C "$T/r" add .; gc -m O; O=$(git -C "$T/r" rev-parse HEAD)
git -C "$T/r" checkout -q -B m "$B"; echo m > "$T/r/m.c"; git -C "$T/r" add .; gc -m M
git -C "$T/r" -c user.name=t -c user.email=t@t merge -q --no-edit refs-oscuro >/dev/null; MO=$(git -C "$T/r" rev-parse HEAD)
expect REFUSED "the refs-oscuro tip itself" "$O" X0X_OSCURO_COMMITS=
expect REFUSED "a merge of refs-oscuro" "$MO" X0X_OSCURO_COMMITS=
expect OK "base with a refs-oscuro branch in the repository" "$B" X0X_OSCURO_COMMITS=
git -C "$T/r" branch -q -m refs-oscuro old-o
expect REFUSED "X0X_OSCURO_COMMITS names the commit (no branch)" "$MO" X0X_OSCURO_COMMITS="$(echo "$O" | cut -c1-7)"
n=$((n + 1)); out=$(sh "$CK" --repo "$T/r/.git" 0000000 2>&1); rc=$?
[ "$rc" = 2 ] && echo "ok   unknown commit: exit 2" || { echo "FAIL unknown commit: rc=$rc $out"; fails=$((fails + 1)); }
echo "release tree: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
