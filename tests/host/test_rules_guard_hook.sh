#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# tools/update.hook part 1b (check 28a, rules = code): on a scratch bare repository with the hooks of this tree
# (tools/install_server_hooks.sh). A push to smk37 that changes docs/rules/ or USER_PREFERENCES.md is accepted only as
# a merge of a lane/dk-rules-* tip (first parent = old smk37, no rule change in the merge) or as a fast-forward to that tip.
# Fixture: tests/host/fixtures/rules_guard/bad-rule.md (a bad rule text pushed directly to smk37: rejected).
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; F="$TOP/tests/host/fixtures/rules_guard"
T=$(mktemp -d "${TMPDIR:-/tmp}/rulesguard.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
B="$T/box.git"; C="$T/c"
git init -q --bare "$B" && git -C "$B" config x0x37.workDir "$T/wd" && git -C "$B" config x0x37.skipLog "$T/skips.log" || { echo "FAIL setup"; exit 1; }
gc() { git -C "$C" -c user.name=t -c user.email=t@t commit -q "$@"; }
git init -q "$C" && git -C "$C" config user.name t && git -C "$C" config user.email t@t && mkdir -p "$C/docs/rules" && cp "$F/good-rule.md" "$C/docs/rules/install.md" && echo prefs > "$C/docs/USER_PREFERENCES.md" &&
    echo r > "$C/README.md" && git -C "$C" add . && gc -m base && git -C "$C" branch -M smk37 && git -C "$C" remote add box "$B" &&
    git -C "$C" push -q box smk37 2>/dev/null || { echo "FAIL setup: first push"; exit 1; }
bash "$TOP/tools/install_server_hooks.sh" "$B" >/dev/null || { echo "FAIL setup: hooks"; exit 1; }   # after the first smk37 push
push() {  # push <accept|reject> <name> <refspec...>
    want=$1; name=$2; shift 2; n=$((n + 1))
    out=$(git -C "$C" push box "$@" 2>&1); rc=$?
    if { [ "$want" = accept ] && [ $rc = 0 ]; } || { [ "$want" = reject ] && [ $rc != 0 ] && echo "$out" | grep -q 'REJECTED'; }; then
        echo "ok   $name: $(echo "$out" | grep -m1 -E 'update hook' | sed 's/^remote: //')"
    else echo "FAIL $name (want $want, rc=$rc)"; echo "$out" | sed 's/^/     /'; fails=$((fails + 1)); fi
}
co() { git -C "$C" checkout -q "$@"; }
# 1. a bad rule text directly on smk37
co smk37; cp "$F/bad-rule.md" "$C/docs/rules/install.md"; gc -am "Change rule 7"; push reject "direct rule change on smk37" smk37
git -C "$C" reset -q --hard box/smk37
# 2. USER_PREFERENCES.md directly on smk37
echo more >> "$C/docs/USER_PREFERENCES.md"; gc -am "Add a preference"; push reject "direct USER_PREFERENCES.md change" smk37
git -C "$C" reset -q --hard box/smk37
# 3. no rule change: not checked
echo r2 >> "$C/README.md"; gc -am "Change the readme"; push accept "smk37 change without rule files" smk37
# 4. the lane, then a merge of its tip
co -b lane/dk-rules-a smk37; echo "Owner fact (2026-10-17 02:37)." >> "$C/docs/rules/install.md"; gc -am "Add the fact"
push accept "lane/dk-rules-a push" lane/dk-rules-a
co smk37; echo r3 >> "$C/README.md"; gc -am "Readme again"; push accept "smk37 moves" smk37
git -C "$C" merge -q --no-ff --no-edit lane/dk-rules-a; push accept "merge of the lane tip" smk37
# 5. an evil merge: the merge commit adds its own rule change
co -b lane/dk-rules-b smk37; echo "Second fact." >> "$C/docs/rules/install.md"; gc -am "Second fact"; push accept "lane/dk-rules-b push" lane/dk-rules-b
co smk37; echo r4 >> "$C/README.md"; gc -am "Readme 4"; push accept "smk37 moves again" smk37
git -C "$C" merge -q --no-ff --no-commit lane/dk-rules-b; cp "$F/bad-rule.md" "$C/docs/rules/install.md"; git -C "$C" add -A; gc -m "Merge b"
push reject "merge with its own rule change" smk37
git -C "$C" reset -q --hard box/smk37
# 6. a merge of a branch that is not lane/dk-rules-*
co -b lane/mb-x smk37; cp "$F/bad-rule.md" "$C/docs/rules/install.md"; gc -am "Rule on a builder lane"; push accept "lane/mb-x push" lane/mb-x
co smk37; git -C "$C" merge -q --no-ff --no-edit lane/mb-x; push reject "merge of lane/mb-x" smk37
git -C "$C" reset -q --hard box/smk37
# 7. a merge of a lane commit that is not the lane tip (the lane moved on)
co lane/dk-rules-b; OLDTIP=$(git -C "$C" rev-parse HEAD); echo "Third." >> "$C/docs/rules/install.md"; gc -am "Third fact"; push accept "lane/dk-rules-b moves" lane/dk-rules-b
co smk37; git -C "$C" merge -q --no-ff --no-edit "$OLDTIP"; push reject "merge of an old lane commit" smk37
git -C "$C" reset -q --hard box/smk37
# 8. fast-forward to the lane tip
co -b lane/dk-rules-c smk37; echo "Fourth." >> "$C/docs/rules/install.md"; gc -am "Fourth fact"; push accept "lane/dk-rules-c push" lane/dk-rules-c
push accept "fast-forward smk37 to the lane tip" lane/dk-rules-c:smk37
echo "rules guard hook: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
