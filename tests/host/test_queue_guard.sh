#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/require_gate_pass.sh, check 6 (core rule 6A, install rule 6): the queue file QUEUE.md in the results folder.
# Each open line "- [ ] <commit> <text>" must name a commit that is the install commit or an ancestor of it.
# A temp repository: A, then B on A (the install commit, with a valid gate PASS), and C on A (not in B).
#   Each case sets X0X_QUEUE_FILE to a temp file (commit.md rule 9), so the live QUEUE.md never changes a result.
#   no queue file -> GATE OK (an install of a gated commit under the old rules still runs)
#   open A, open B, done C -> GATE OK.   open C -> REFUSED.   open unknown commit -> REFUSED.   open, no commit -> REFUSED.
#   fixtures/queue_guard/no-commit.txt (the form of the live QUEUE.md: open lines "- [ ] pending ..." with no commit)
#     -> REFUSED, exit 3 (Orchestrator 03:01: lock in this behaviour).
#   tools/sync_results_to_mac.sh copies QUEUE.md to the Mac copy, and the guard on the copy refuses open C.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
unset X0X_GATE_LOGS_MAP X0X_SYNC_DIR X0X_QUEUE_FILE
G="$(cd "$(dirname "$0")/../.." && pwd)/tools/require_gate_pass.sh"
SY="$(dirname "$G")/sync_results_to_mac.sh"
T=$(mktemp -d "${TMPDIR:-/tmp}/qguard.XXXXXX"); trap 'chmod -R u+w "$T" 2>/dev/null; rm -rf "$T"' EXIT
QF=$(mktemp "$T/queue.XXXXXX")   # commit.md rule 9: each case gives X0X_QUEUE_FILE = a temp file, never the live QUEUE.md
fails=0; n=0
expect() {  # expect <REFUSED|GATE OK> <name> [results folder] [queue file] [queue refusal reason]
    want=$1; name=$2; res=${3:-$T/res}; qf=${4:-$QF}; reason=${5:-}; n=$((n + 1))
    out=$(X0X_QUEUE_FILE="$qf" X0X_RESULTS_DIR="$res" X0X_REPO="$T/repo" bash "$G" "$B" 2>&1); rc=$?
    case "$want" in REFUSED) wrc=3;; *) wrc=0;; esac
    case "$out" in "$want "*)
        if [ "$rc" = "$wrc" ]; then
            case "$want:$out" in
                REFUSED:*)
                    # A provenance failure must not satisfy a queue refusal case.
                    case "$out" in *": open queue line"*"$reason"*) [ -n "$reason" ] && { echo "ok   $name: $out"; return; };; esac ;;
                *) echo "ok   $name: $(echo "$out" | tail -1)"; return ;;
            esac
        fi ;;
    esac
    echo "FAIL $name: rc=$rc out=$out"; fails=$((fails + 1))
}
gc() { git -c user.name=t -c user.email=t@t commit -q "$@"; }
mkdir -p "$T/repo" "$T/res" && cd "$T/repo" && git init -q . && echo a > a.c && git add . && gc -m A && \
    A=$(git rev-parse HEAD) && echo b > b.c && git add . && gc -m B && B=$(git rev-parse HEAD) && \
    git checkout -q -b side "$A" && echo c > c.c && git add . && gc -m C && C=$(git rev-parse HEAD) && \
    cd - >/dev/null || { echo "FAIL setup"; exit 1; }
S=$(echo "$B" | cut -c1-7)
if command -v sha1sum >/dev/null 2>&1; then SHA1=sha1sum; else SHA1="shasum -a 1"; fi
TREE=$(git -C "$T/repo" ls-tree -r --full-tree "$B" | awk -F '\t' '$2 !~ /\.md$/' | $SHA1 | cut -c1-40)
T12=$(echo "$TREE" | cut -c1-12)
L="$T/work/$S.logs"; mkdir -p "$L"
printf 'gate: took the gate lock after 0 s\n' > "$L/gate.log"; printf 'ctl.img file crc32 00000000, 1 B\n' > "$L/crc.txt"
INPUT_ID=$(python3 - "$G" "$T/repo" "$B" "$L" <<'PYID'
import json, sys
from pathlib import Path
sys.path.insert(0, str(Path(sys.argv[1]).parent))
import validation_inputs as v
# Synthetic provenance only for this disposable queue fixture; no validation run.
payload = {'schema': v.SCHEMA, 'mode': 'gate', 'source': v.source_tree(sys.argv[2], sys.argv[3])}
data = {'schema': v.SCHEMA, 'inputs': payload, 'sha256': v.digest(v.encoded(payload))}
(Path(sys.argv[4])/'validation-inputs.json').write_text(json.dumps(data))
print(data['sha256'])
PYID
)
{ echo "# Gate result for tree $TREE"; echo; echo "Result: PASS"; echo; echo "- Commit: $B"
  echo "- Validation inputs: $INPUT_ID"
  echo "- Tree (SHA-1 of git ls-tree -r without *.md): $TREE"; echo "- Worktree: $T/work/$S. Logs: $L."
  echo; echo "## CRC manifest"; echo; cat "$L/crc.txt"; } > "$T/res/$TREE.md"
printf '| Commit | Tree | Git tree | Result | Result file | Time |\n|---|---|---|---|---|---|\n| %s | %s | 000000000000 | PASS | %s.md | 2026-10-17 02:00 +10 |\n' "$S" "$T12" "$TREE" > "$T/res/index.md"
queue() { { echo "# Queue"; echo; for l in "$@"; do echo "$l"; done; } > "$QF"; }
expect "GATE OK" "no queue file" "$T/res" "$T/absent-queue.md"
queue "- [ ] $A first change" "- [ ] $(echo "$B" | cut -c1-9) the install commit" "- [x] $C done, not in the set"
expect "GATE OK" "open lines in the history, done line outside"
queue "- [ ] $A first change" "- [ ] $(echo "$C" | cut -c1-7) a change that is not in the install commit"
expect REFUSED "open line not an ancestor" "$T/res" "$QF" "is not in the install commit"
queue "- [ ] 0123456789abcdef0123 a commit that is not in the repository"
expect REFUSED "open line with an unknown commit" "$T/res" "$QF" "is not in $T/repo"
queue "- [ ] a change with no commit"
expect REFUSED "open line with no commit" "$T/res" "$QF" "with no commit"
cp "$(dirname "$G")/../tests/host/fixtures/queue_guard/no-commit.txt" "$QF"
expect REFUSED "fixture no-commit.txt: open lines with no commit (exit 3)" "$T/res" "$QF" "with no commit"
# the Mac copy: --pack takes QUEUE.md, --unpack writes it, the guard reads it there
queue "- [ ] $C a change that is not in the install commit"; cp "$QF" "$T/res/QUEUE.md"; n=$((n + 1))
if X0X_RESULTS_DIR="$T/res" X0X_SYNC_DIR="$T/sync" bash "$SY" --pack >/dev/null 2>&1 && \
   X0X_RESULTS_DIR="$T/mac" bash "$SY" --unpack "$T/sync/latest.tar" >/dev/null 2>&1 && [ -f "$T/mac/QUEUE.md" ]; then
    echo "ok   sync copies QUEUE.md"
else echo "FAIL sync does not copy QUEUE.md"; fails=$((fails + 1)); fi
expect REFUSED "guard on the Mac copy, open line not an ancestor" "$T/mac" "$T/mac/QUEUE.md" "is not in the install commit"
# commit.md rule 9: each test that runs require_gate_pass.sh sets X0X_QUEUE_FILE (bad fixture: fixtures/queue_guard/leaky_test.sh)
TD="$(dirname "$G")/../tests/host"
scanq() { for f in "$@"; do grep -qE 'bash +"?\$(G|Q)"? |require_gate_pass\.sh"? ' "$f" && grep -qE '(^|[^.])require_gate_pass\.sh' "$f" && ! grep -q 'X0X_QUEUE_FILE' "$f" && echo "$f"; done; }
n=$((n + 1)); r=$(scanq "$TD/fixtures/queue_guard/leaky_test.sh")
if [ -n "$r" ]; then echo "ok   scan flags the bad fixture leaky_test.sh"; else echo "FAIL scan does not flag fixtures/queue_guard/leaky_test.sh"; fails=$((fails + 1)); fi
n=$((n + 1)); r=$(for f in "$TD"/*.sh; do grep -qE 'bash +"?\$(G|Q)"? ' "$f" || continue; scanq "$f"; done)
if [ -z "$r" ]; then echo "ok   scan: each test that runs require_gate_pass.sh sets X0X_QUEUE_FILE"; else echo "FAIL scan: no X0X_QUEUE_FILE in: $r"; fails=$((fails + 1)); fi
echo "queue guard: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
