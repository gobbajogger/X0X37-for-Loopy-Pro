#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# tools/status.sh, check 12 of rules v4: the commits on lane/<agent>-* branches count as agent output
# (the false STALL at 02:03: lane/mb-* commits did not count). A temp bare repository with integration and the
# lanes lane/mb-x, lane/sb-x, lane/opt-x, lane/btr-x and lane/dk-rules-x. status.sh writes to a temp folder
# (X0X_REPO, X0X_STATUS_OUT, X0X_STATUS_TMP, X0X_RESULTS_DIR, X0X_GATE_LOCK). Each agent row of the Agents table
# must name its lane in the "Last commit" column.
set -u
export X0X_TEST_RUN=1   # check 30: status.sh refuses the live folder
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; S="$TOP/tools/status.sh"
T=$(mktemp -d "${TMPDIR:-/tmp}/statuslanes.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
B="$T/box.git"; C="$T/c"
git init -q --bare "$B" && git init -q "$C" && git -C "$C" config user.name t && git -C "$C" config user.email t@t || { echo "FAIL setup"; exit 1; }
echo a > "$C/a" && git -C "$C" add . && git -C "$C" commit -qm base && git -C "$C" branch -M integration
for l in mb sb opt btr dk-rules; do
    git -C "$C" checkout -q -b "lane/$l-x" integration && echo "$l" > "$C/$l" && git -C "$C" add . && git -C "$C" commit -qm "Lane $l"
done
git -C "$C" push -q "$B" --all 2>/dev/null || { echo "FAIL setup push"; exit 1; }
mkdir -p "$T/out" "$T/tmp" "$T/res" && : > "$T/gate.lock"
out=$(X0X_REPO="$B" X0X_STATUS_OUT="$T/out" X0X_STATUS_TMP="$T/tmp" X0X_RESULTS_DIR="$T/res" X0X_GATE_LOCK="$T/gate.lock" bash "$S" 2>&1); rc=$?
n=$((n + 1)); if [ $rc = 0 ] && [ -s "$T/out/status.md" ]; then echo "ok   status.sh ran: $out"; else echo "FAIL status.sh rc=$rc: $out"; fails=$((fails + 1)); fi
for p in "Firmware Main Builder|lane/mb-x" "Second Builder|lane/sb-x" "Optimizer|lane/opt-x" "Build and Test Runner|lane/btr-x" "Docs Keeper|lane/dk-rules-x"; do
    a=${p%%|*}; l=${p#*|}; n=$((n + 1))
    row=$(grep "^| $a |" "$T/out/status.md" 2>/dev/null)
    if echo "$row" | grep -qF "($l)"; then echo "ok   $a: output from $l"; else echo "FAIL $a: no ($l) in the row: $row"; fails=$((fails + 1)); fi
done
echo "status lanes: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
