#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/prefs_lint.py (check 27): each owner-rule entry of USER_PREFERENCES.md names a rule of docs/rules/ that
# quotes or covers it. Fixtures: tests/host/fixtures/prefs_lint/ (good.md, bad.md, rules/).
#   good.md -> "PREFS OK 3 entries", exit 0 (the product section and the undated item are not checked).
#   bad.md  -> "PREFS FAIL 5 of 5 entries", exit 1: no reference (with the hint of the rule that quotes 18:41), a rule
#              number that does not exist, a wrong file, a rule that does not cover the entry, a time "owner rule 01:57".
#   --git mode on a temp repository (docs/USER_PREFERENCES.md, docs/rules/): the same results.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; L="$TOP/tools/prefs_lint.py"; F="$TOP/tests/host/fixtures/prefs_lint"
T=$(mktemp -d "${TMPDIR:-/tmp}/prefslint.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
check() {  # check <name> <want rc> <want text (grep -F)> <command...>
    name=$1; wrc=$2; want=$3; shift 3; n=$((n + 1))
    out=$("$@" 2>&1); rc=$?
    if [ "$rc" = "$wrc" ] && printf '%s\n' "$out" | grep -qF -- "$want"; then echo "ok   $name"
    else echo "FAIL $name: rc=$rc (want $wrc), want \"$want\""; printf '%s\n' "$out" | sed 's/^/     /'; fails=$((fails + 1)); fi
}
check "good fixture" 0 "PREFS OK 3 entries" python3 "$L" --rules "$F/rules" "$F/good.md"
check "bad fixture: count" 1 "PREFS FAIL 5 of 5 entries" python3 "$L" --rules "$F/rules" "$F/bad.md"
check "bad: no reference, hint" 1 "NO STEP line 5 2026-10-16 18:41: 2026-10-16 18:41 (owner): Font size is not a feature. The entry names no rule.; quoted by style.md rule 15" python3 "$L" --rules "$F/rules" "$F/bad.md"
check "bad: rule 99 does not exist" 1 "NO STEP line 6 2026-10-17 01:57" python3 "$L" --rules "$F/rules" "$F/bad.md"
check "bad: commit.md rule 7 (wrong file)" 1 "NO STEP line 7 2026-10-17 02:00" python3 "$L" --rules "$F/rules" "$F/bad.md"
check "bad: rule 7 does not cover" 1 "NOT COVERED line 11 2026-10-17 02:05: install.md rule 7" python3 "$L" --rules "$F/rules" "$F/bad.md"
check "bad: owner rule 01:57 is a time" 1 "NO STEP line 12 2026-10-17 02:10" python3 "$L" --rules "$F/rules" "$F/bad.md"
R="$T/repo"; mkdir -p "$R/docs/rules" && cp "$F/rules/"*.md "$R/docs/rules/" && cp "$F/good.md" "$R/docs/USER_PREFERENCES.md" && \
    git -C "$R" init -q && git -C "$R" add . && git -C "$R" -c user.name=t -c user.email=t@t commit -qm good && \
    cp "$F/bad.md" "$R/docs/USER_PREFERENCES.md" && git -C "$R" -c user.name=t -c user.email=t@t commit -qam bad || { echo "FAIL setup"; exit 1; }
check "--git good" 0 "PREFS OK 3 entries" python3 "$L" --git "$R" HEAD^
check "--git bad" 1 "PREFS FAIL 5 of 5 entries" python3 "$L" --git "$R" HEAD
check "no rule files" 2 "PREFS ERROR" python3 "$L" --rules "$T/none" "$F/good.md"
echo "prefs lint: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
