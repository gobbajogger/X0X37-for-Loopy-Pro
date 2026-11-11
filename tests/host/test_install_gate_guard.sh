#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/require_gate_pass.sh (the install guard) with a temp index.md and a temp results folder
# (X0X_RESULTS_DIR). Each refusal case must print "REFUSED" with exit code 3; the gate PASS case
# must print "GATE OK" with exit code 0. A real result (9a733fe, tree 1d3c048d...) is also checked
# when /workspace/x0x37.git and /workspace/x0x37-results are on this computer.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE   # a hook sets these; a scratch repository must not use the repository of the hook
unset X0X_GATE_LOGS_MAP X0X_SYNC_DIR
G="$(cd "$(dirname "$0")/../.." && pwd)/tools/require_gate_pass.sh"
T=$(mktemp -d "${TMPDIR:-/tmp}/igg.XXXXXX"); trap 'rm -rf "$T"' EXIT
# commit.md rule 9 (tests use temp files): the queue file of every case is a temp file (empty: no open lines).
# The live QUEUE.md never reaches this test. tests/host/test_queue_guard.sh tests the queue.
X0X_QUEUE_FILE=$(mktemp "$T/queue.XXXXXX"); export X0X_QUEUE_FILE
fails=0; n=0
expect() {  # expect <REFUSED|GATE OK> <name> <args...>
    want=$1; name=$2; shift 2; n=$((n + 1))
    out=$(X0X_RESULTS_DIR="$T/res" X0X_REPO="$T/repo" bash "$G" "$@" 2>&1); rc=$?
    case "$want" in REFUSED) wrc=3;; *) wrc=0;; esac
    case "$out" in "$want "*) [ "$rc" = "$wrc" ] && { echo "ok   $name: $out"; return; };; esac
    echo "FAIL $name: rc=$rc out=$out"; fails=$((fails + 1))
}
# a temp repository and its gate tree key (the same command as tools/gate.sh)
mkdir -p "$T/repo" "$T/res" && cd "$T/repo" && git init -q . && echo a > a.c && echo doc > README.md && \
    git add . && git -c user.name=t -c user.email=t@t commit -qm one && cd - >/dev/null || { echo "FAIL setup"; exit 1; }
H=$(git -C "$T/repo" rev-parse HEAD); S=$(echo "$H" | cut -c1-7)
if command -v sha1sum >/dev/null 2>&1; then SHA1=sha1sum; else SHA1="shasum -a 1"; fi
TREE=$(git -C "$T/repo" ls-tree -r --full-tree "$H" | awk -F '\t' '$2 !~ /\.md$/' | $SHA1 | cut -c1-40)
T12=$(echo "$TREE" | cut -c1-12); OTHER=0123456789abcdef0123456789abcdef01234567
printf 'fwsc bytes\n' > "$T/good.fwsc"; printf 'ctl bytes\n' > "$T/ctl.img"; printf 'other\n' > "$T/bad.fwsc"
crc() { python3 -c 'import sys, zlib; print("%08x" % (zlib.crc32(open(sys.argv[1], "rb").read()) & 0xffffffff))' "$1"; }
FC=$(crc "$T/good.fwsc"); CC=$(crc "$T/ctl.img")
L="$T/work/$S.logs"; mkdir -p "$L"
printf 'gate: took the gate lock after 0 s\ngate: released the gate lock; the tests took 1 s\n' > "$L/gate.log"
printf 'ctl.img file crc32 %s, 10 B\nbuild/x0x-smk37-elite.fwsc crc32 %s, 11 B\n' "$CC" "$FC" > "$L/crc.txt"
INPUT_ID=$(python3 - "$G" "$T/repo" "$L" <<'PYID'
import json, sys
from pathlib import Path
sys.path.insert(0, str(Path(sys.argv[1]).parent))
import validation_inputs as v
# Synthetic provenance only; no validation run or device involved.
payload = {'schema': v.SCHEMA, 'mode': 'gate', 'source': v.source_tree(sys.argv[2], 'HEAD')}
data = {'schema': v.SCHEMA, 'inputs': payload, 'sha256': v.digest(v.encoded(payload))}
(Path(sys.argv[3])/'validation-inputs.json').write_text(json.dumps(data))
print(data['sha256'])
PYID
)
result() {  # result <file> <Result line> <tree>
    { echo "# Gate result for tree $3"; echo; echo "$2"; echo; echo "- Commit: $H"
      echo "- Validation inputs: $INPUT_ID"
      echo "- Tree (SHA-1 of git ls-tree -r without *.md): $3"; echo "- Worktree: $T/work/$S. Logs: $L."
      echo; echo "## CRC manifest"; echo; echo '```'; cat "$L/crc.txt"; echo '```'; } > "$T/res/$1"; }
index() { printf '| Commit | Tree | Git tree | Result | Result file | Time |\n|---|---|---|---|---|---|\n' > "$T/res/index.md"
          for r in "$@"; do echo "$r" >> "$T/res/index.md"; done; }
row() { echo "| $S | $T12 | 000000000000 | $1 | $2 | 2026-10-16 22:00 +10 |"; }

index; expect REFUSED "no row" "$H"
index "$(row 'PASS' "$TREE.md")"; expect REFUSED "manual row, no result file" "$H"
result "$TREE.md" "Result: PASS" "$TREE"
index "$(row 'PASS (backfill)' "$TREE.md")"; expect REFUSED "hand-written PASS text" "$H"
result "$TREE.trial-1.md" "Result: TRIAL PASS (a trial run with --trial: not a release PASS)" "$TREE"
index "$(row 'TRIAL PASS' "$TREE.trial-1.md")"; expect REFUSED "TRIAL PASS" "$H"
index "$(row 'VOID (setup error: no toolchain; not a test result)' "$TREE.md")"; expect REFUSED "VOID" "$H"
index "$(row 'FAIL' "$TREE.md")" "$(row 'KILLED' "$TREE.md")"; expect REFUSED "FAIL and KILLED" "$H"
result "$OTHER.md" "Result: PASS" "$OTHER"
index "$(row 'PASS' "$OTHER.md")"; expect REFUSED "PASS row, result file of another tree" "$H"
index "$(row 'PASS' "$TREE.md")"
mv "$L/gate.log" "$L/gate.keep"; printf 'build output only\n' > "$L/gate.log"
expect REFUSED "gate.log without gate lines" "$H"; mv "$L/gate.keep" "$L/gate.log"
expect REFUSED "CRC mismatch" "$H" --fwsc "$T/bad.fwsc" --ctl "$T/ctl.img"
expect "GATE OK" "gate PASS" "$H" --fwsc "$T/good.fwsc" --ctl "$T/ctl.img"
index "$(row 'PASS' "$TREE.md")" "$(row 'PASS (result used again)' "$TREE.md")"; expect "GATE OK" "PASS (result used again)" "$H"
# tools/sync_results_to_mac.sh: pack on the box, unpack on the "Mac" (a temp folder), then the guard
# uses only the copy (the original logs folder is moved away). All copied files must be read-only.
SY="$(dirname "$G")/sync_results_to_mac.sh"; n=$((n + 1))
p=$(X0X_RESULTS_DIR="$T/res" X0X_SYNC_DIR="$T/sync" bash "$SY" --pack 2>&1) && \
    u=$(X0X_RESULTS_DIR="$T/mac" bash "$SY" --unpack "$T/sync/latest.tar" 2>&1) && mv "$L" "$L.away" && \
    out=$(X0X_RESULTS_DIR="$T/mac" X0X_REPO="$T/repo" bash "$G" "$H" --fwsc "$T/good.fwsc" --ctl "$T/ctl.img" 2>&1); rc=$?
mv "$L.away" "$L" 2>/dev/null
rw=$(find "$T/mac" -type f -perm -u+w ! -path '*/.unpack.*' | wc -l | tr -d ' ')
case "$out" in "GATE OK $S $T/mac/"*) if [ "$rc" = 0 ] && [ "$rw" = 0 ]; then echo "ok   sync pack + unpack, guard on the copy: $out"
    else echo "FAIL sync: rc=$rc writable=$rw $out"; fails=$((fails + 1)); fi ;;
*) echo "FAIL sync: rc=$rc $p $u $out"; fails=$((fails + 1)) ;; esac
chmod -R u+w "$T/mac" 2>/dev/null
# fm1_install.py: a real install with no --commit stops before it opens a device (exit 9)
n=$((n + 1)); out=$(python3 "$(dirname "$G")/fm1_install.py" "$T/good.fwsc" 2>&1); rc=$?
if [ "$rc" = 9 ] && echo "$out" | grep -q REFUSED; then echo "ok   fm1_install.py without --commit: exit 9"; else echo "FAIL fm1_install.py without --commit: rc=$rc $out"; fails=$((fails + 1)); fi
# two configs (dev X0X_DEV_PADCAL=1, rel 0): the CRC must be in the lines of the config that --config names.
# A result with one config (above) counts only as dev. The index row has the Configs column.
printf 'release bytes\n' > "$T/rel.fwsc"; RC=$(crc "$T/rel.fwsc")
printf 'ctl.img file crc32 %s, 10 B\nbuild/x0x-smk37-elite.fwsc crc32 %s, 11 B\nrel/build/x0x-smk37-elite.fwsc crc32 %s, 14 B\n' "$CC" "$FC" "$RC" > "$L/crc.txt"
result "$TREE-2.md" "Result: PASS" "$TREE"
awk -v c="- Config CRCs: dev elite $FC, ble -; rel elite $RC, ble -" '/^- Worktree: /{print c} {print}' "$T/res/$TREE-2.md" > "$T/r2" && mv "$T/r2" "$T/res/$TREE-2.md"
index "| $S | $T12 | 000000000000 | PASS | $TREE-2.md | 2026-10-17 00:30 +10 | dev elite $FC; rel elite $RC |"
expect "GATE OK" "two configs: dev fwsc, --config dev" "$H" --fwsc "$T/good.fwsc" --config dev --ctl "$T/ctl.img"
expect "GATE OK" "two configs: dev fwsc, no --config (dev)" "$H" --fwsc "$T/good.fwsc"
expect "GATE OK" "two configs: rel fwsc, --config rel" "$H" --fwsc "$T/rel.fwsc" --config rel
expect REFUSED "two configs: rel fwsc, --config dev (swapped)" "$H" --fwsc "$T/rel.fwsc" --config dev
expect REFUSED "two configs: dev fwsc, --config rel (swapped)" "$H" --fwsc "$T/good.fwsc" --config rel
expect REFUSED "two configs: unknown CRC, --config rel" "$H" --fwsc "$T/bad.fwsc" --config rel
index "$(row 'PASS' "$TREE.md")"
expect REFUSED "one-config result, --config rel" "$H" --fwsc "$T/good.fwsc" --config rel
n=$((n + 1)); out=$(X0X_RESULTS_DIR="$T/res" X0X_REPO="$T/repo" bash "$G" "$H" --config beta 2>&1); rc=$?
[ "$rc" = 2 ] && echo "ok   --config beta: exit 2" || { echo "FAIL --config beta: rc=$rc $out"; fails=$((fails + 1)); }
# Legacy evidence and damaged provenance must fail closed, even with exact CRCs.
mv "$L/validation-inputs.json" "$L/validation-inputs.keep"
expect REFUSED "missing validation identity" "$H"
mv "$L/validation-inputs.keep" "$L/validation-inputs.json"
printf 'changed' >> "$L/validation-inputs.json"
expect REFUSED "damaged validation identity" "$H"
echo "install gate guard: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
