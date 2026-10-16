#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# tools/status.sh, checks 27 and 29 of rules v4, on a temp bare repository (integration, smk37) and a temp results folder.
#   29a: the installed hash of ROADMAP.md on smk37 differs from the last row of INSTALLED.tsv -> "ROADMAP INSTALLED MISMATCH".
#   29b: an integration commit after the last install names a BEHAVIOR.md item (prefix of an item, for example CLP-99)
#        that integration:BEHAVIOR.md does not have -> "BEHAVIOR ITEM MISSING". Other words such as SHA-1 do not count.
#   27:  the docs lint (X0X_PREFS_LINT, here a stand-in) fails -> "RULE POINTERS".
# Fixtures: tests/host/fixtures/status/ (BEHAVIOR.md, ROADMAP-good.md, ROADMAP-bad.md, prefs_lint_ok.py, prefs_lint_fail.py).
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; S="$TOP/tools/status.sh"; F="$TOP/tests/host/fixtures/status"
T=$(mktemp -d "${TMPDIR:-/tmp}/statuschk.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
C="$T/c"; git init -q "$C" && git -C "$C" config user.name t && git -C "$C" config user.email t@t || { echo "FAIL setup"; exit 1; }
cp "$F/BEHAVIOR.md" "$C/" && git -C "$C" add . && git -C "$C" commit -qm "Base" && git -C "$C" branch -M integration
A=$(git -C "$C" rev-parse HEAD)
mkdir -p "$T/res"; printf 'time\thash\tfwsc\tctl_crc\tgate_tree\tresult\n2026-10-17 02:25 +10\t%s\tb10.fwsc\t-\t-\tPASS\n' "$A" > "$T/res/INSTALLED.tsv"
run() {  # run <ROADMAP fixture> <lint stand-in> <integration commit subject>: the status.md text
    rm -rf "$T/box.git" "$T/out" "$T/tmp"; git init -q --bare "$T/box.git"; mkdir -p "$T/out" "$T/tmp"; : > "$T/gate.lock"
    git -C "$C" checkout -q integration && git -C "$C" reset -q --hard "$A" && echo "$3" >> "$C/BEHAVIOR.md.log" && git -C "$C" add . && git -C "$C" commit -qm "$3"
    git -C "$C" checkout -q -B smk37 "$A" && mkdir -p "$C/docs" && sed "s/HASH/${A:0:7}/" "$F/$1" > "$C/docs/ROADMAP.md" && git -C "$C" add . && git -C "$C" commit -qm "Docs"
    git -C "$C" push -q -f "$T/box.git" integration smk37 2>/dev/null
    X0X_REPO="$T/box.git" X0X_STATUS_OUTDIR="$T/out" X0X_STATUS_TMP="$T/tmp" X0X_RESULTS_DIR="$T/res" X0X_GATE_LOCK="$T/gate.lock" \
        X0X_PREFS_LINT="$F/$2" bash "$S" >/dev/null 2>&1; cat "$T/out/status.md" 2>/dev/null
}
chk() {  # chk <name> <yes|no> <fixed text> <status text>
    n=$((n + 1)); if printf '%s\n' "$4" | grep -qF -- "$3"; then got=yes; else got=no; fi
    if [ "$got" = "$2" ]; then echo "ok   $1"; else echo "FAIL $1: want $2 \"$3\""; printf '%s\n' "$4" | grep -E '^- ' | head -12 | sed 's/^/     /'; fails=$((fails + 1)); fi
}
BAD=$(run ROADMAP-bad.md prefs_lint_fail.py "Clip pads: new rule CLP-99 and a fix for CLP-18 (SHA-1 table).")
chk "29a: ROADMAP hash differs" yes "**ROADMAP INSTALLED MISMATCH** (check 29): ROADMAP.md on smk37 names b8255f2, the last INSTALLED.tsv row is ${A:0:7}" "$BAD"
chk "29b: CLP-99 not in BEHAVIOR.md" yes "**BEHAVIOR ITEM MISSING** (check 29): CLP-99" "$BAD"
L29=$(printf '%s\n' "$BAD" | grep -F 'BEHAVIOR ITEM MISSING')
chk "29b: SHA-1 is not an item" no "SHA-1" "$L29"
chk "29b: CLP-18 is in BEHAVIOR.md" no "CLP-18" "$L29"
chk "27: lint fails" yes "**RULE POINTERS** (check 27): PREFS FAIL 1 of 3 entries" "$BAD"
GOOD=$(run ROADMAP-good.md prefs_lint_ok.py "Clip pads: a fix for CLP-18.")
chk "29a: ROADMAP hash = INSTALLED.tsv" yes "ROADMAP installed (check 29): ${A:0:7} = the last INSTALLED.tsv row." "$GOOD"
chk "29b: all items there" yes "BEHAVIOR items since the last install (check 29): 1 named, all in BEHAVIOR.md." "$GOOD"
chk "27: lint passes" yes "Rule pointers (check 27): PREFS OK 3 entries." "$GOOD"
chk "good: no flag" no "**ROADMAP" "$GOOD"
echo "status checks: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
