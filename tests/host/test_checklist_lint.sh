#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/checklist_lint.py (check 23, rule 12): an owner test checklist has the 3 header lines (Build with the batch number
# or title with the batch number, Build with the gated hash, Replaces, Checked against decisions with a time), 10 tasks or fewer,
# and each numbered task title is the goal (2 words or more, "Goal:" not necessary).
# Fixtures: tests/host/fixtures/checklist/good.txt and batch10-live.txt (a copy of the owner checklist of batch 10 by Docs Keeper,
# batch number only in the title, no "Goal:"): CHECKLIST OK, exit 0. bad.txt: CHECKLIST FAIL, exit 1 (no Replaces line,
# no batch number, no decision time, a task with no goal title, 11 tasks).
set -u
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; L="$TOP/tools/checklist_lint.py"; F="$TOP/tests/host/fixtures/checklist"
fails=0; n=0
check() {  # check <name> <want rc> <want text> <file>
    n=$((n + 1)); out=$(python3 "$L" "$4" 2>&1); rc=$?
    if [ "$rc" = "$2" ] && printf '%s\n' "$out" | grep -qF -- "$3"; then echo "ok   $1"
    else echo "FAIL $1: rc=$rc (want $2), want \"$3\""; printf '%s\n' "$out" | sed 's/^/     /'; fails=$((fails + 1)); fi
}
check "good" 0 "CHECKLIST OK 2 tasks" "$F/good.txt"
check "live batch 10 checklist" 0 "CHECKLIST OK" "$F/batch10-live.txt"
check "bad: summary" 1 "CHECKLIST FAIL" "$F/bad.txt"
check "bad: no Replaces line" 1 'no header line "Replaces:"' "$F/bad.txt"
check "bad: Build has no batch number" 1 'Build line and title have no batch number' "$F/bad.txt"
check "bad: Checked against decisions has no time" 1 'has no time (HH:MM)' "$F/bad.txt"
check "bad: task without a goal title" 1 'task 2 has no goal title' "$F/bad.txt"
check "bad: 11 tasks" 1 "11 tasks (max 10)" "$F/bad.txt"
echo "checklist lint: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
