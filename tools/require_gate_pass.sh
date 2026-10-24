#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# The install guard: exact gate evidence, or explicit scoped development evidence for dev only.
#   tools/require_gate_pass.sh <commit> [--fwsc FILE] [--config dev|rel] [--ctl FILE] [--dev-record FILE]
# --dev-record accepts scoped DEV CHECKED evidence only for config dev; queue checks still apply.
# Without that option the full gate path is unchanged.
# Checks, in this order (the first failed check stops it):
#   1. The tree key of <commit>, the same as tools/gate.sh: the SHA-1 of "git ls-tree -r --full-tree"
#      without the *.md files.
#   2. index.md has a row for that tree with the result "PASS", "PASS (result used again)" or
#      "PASS (force: <reason>)". TRIAL, VOID, FAIL, KILLED and hand-written rows (for example
#      "PASS (backfill)") do not count. The newest row that passes checks 3 to 5 is used.
#   3. The result file of the row is in the results folder, has the line "Result: PASS" and the full tree.
#   4. The logs folder that the result file names has gate.log with "gate:" lines (written by the gate)
#      and crc.txt (the CRC manifest). The result file has the "## CRC manifest" section.
#   5. With --fwsc / --ctl: the CRC32 of the file is in the CRC manifest of the result file
#      (a .fwsc line of the config, or the "ctl.img file crc32" line).
#      --config dev [default]: a build/*.fwsc line (the dev build, X0X_DEV_PADCAL=1).
#      --config rel: a rel/build/*.fwsc line (the release build, X0X_DEV_PADCAL=0). Only a result of the gate with
#      two configs (the line "- Config CRCs:") has rel lines. An older result (one config) counts only as dev.
#      A CRC of the other config, or a CRC that is not in the manifest, is REFUSED.
#   6. The queue file (core rule 6A, install rule 6: install only the full set): $X0X_RESULTS_DIR/QUEUE.md has one
#      line for each queued change. An open line is "- [ ] <commit> <text>", a done line is "- [x] <commit> <text>".
#      <commit> is the first word, 7 to 40 hex characters. Each open line must name <commit> itself or an ancestor
#      of it. An open line with no commit, or with a commit that is not in the repository, is REFUSED.
#      No QUEUE.md: no queue check. With QUEUE.md, the GATE OK line ends with "; queue: <n> open lines in <commit>".
# Output: "GATE OK <hash> <result file>" (exit 0), or "REFUSED <hash>: <reason>" (exit 3). Bad arguments: exit 2.
# Environment (defaults in brackets):
#   X0X_RESULTS_DIR [/workspace/x0x37-results; if that folder is not there: $HOME/Documents/x0x37/gate-results,
#                    the copy that tools/sync_results_to_mac.sh --unpack makes on the Mac]
#   X0X_REPO [the git repository of the current folder, else /workspace/x0x37.git]
#   X0X_QUEUE_FILE [$X0X_RESULTS_DIR/QUEUE.md]
#   X0X_GATE_LOGS_MAP ["" ; "FROM=TO" replaces the prefix FROM of the logs path with TO, for a copy on another computer]
#   If the logs folder is not there, the guard also looks in $X0X_RESULTS_DIR/logs/<logs path> (the layout of
#   tools/sync_results_to_mac.sh).
# Runs with bash 3.2 (macOS) and Linux. Read only: it changes no file.
set -u
RESULTS=${X0X_RESULTS_DIR:-/workspace/x0x37-results}
[ -n "${X0X_RESULTS_DIR:-}" ] || [ -d "$RESULTS" ] || RESULTS="$HOME/Documents/x0x37/gate-results"
C=""; FWSC=""; CTL=""; CONFIG=dev; DEV_RECORD=""
USAGE="usage: $0 <commit> [--fwsc FILE] [--config dev|rel] [--ctl FILE] [--dev-record FILE]"
while [ $# -gt 0 ]; do
    case "$1" in
    --fwsc) FWSC=${2:-}; shift ;;
    --ctl) CTL=${2:-}; shift ;;
    --dev-record) DEV_RECORD=${2:-}; [ -n "$DEV_RECORD" ] || { echo "$USAGE" >&2; exit 2; }; shift ;;
    --config) CONFIG=${2:-}; shift; case "$CONFIG" in dev|rel) ;; *) echo "$USAGE" >&2; exit 2 ;; esac ;;
    -*) echo "$USAGE" >&2; exit 2 ;;
    *) [ -z "$C" ] && C=$1 || { echo "$USAGE" >&2; exit 2; } ;;
    esac
    shift
done
[ -n "$C" ] || { echo "$USAGE" >&2; exit 2; }
refuse() { echo "REFUSED ${S:-$C}: $1"; exit 3; }
if [ -n "${X0X_REPO:-}" ]; then REPO=$X0X_REPO
elif git rev-parse --git-dir >/dev/null 2>&1; then REPO=$(git rev-parse --show-toplevel 2>/dev/null || git rev-parse --git-dir)
else REPO=/workspace/x0x37.git; fi
H=$(git -C "$REPO" rev-parse --verify --quiet "$C^{commit}") || refuse "unknown commit in $REPO"
S=$(echo "$H" | cut -c1-7)
if command -v sha1sum >/dev/null 2>&1; then SHA1="sha1sum"; else SHA1="shasum -a 1"; fi
# 1. the tree key (the same command as tools/gate.sh)
TREE=$(git -C "$REPO" ls-tree -r --full-tree "$H" | awk -F '\t' '$2 !~ /\.md$/' | $SHA1 | cut -c1-40)
T12=$(echo "$TREE" | cut -c1-12)
INDEX="$RESULTS/index.md"
# 6. the queue file: each open line names <commit> or an ancestor of it
queue_check() {
    local Q=${X0X_QUEUE_FILE:-$RESULTS/QUEUE.md} line w qh nl=0
    QNOTE=""; [ -f "$Q" ] || return 0
    while IFS= read -r line || [ -n "$line" ]; do
        case "$line" in "- [ ] "*|"* [ ] "*) ;; *) continue ;; esac
        nl=$((nl + 1))
        w=$(printf '%s\n' "${line#?????}" | awk '{print $1}')
        echo "$w" | grep -qE '^[0-9a-f]{7,40}$' || refuse "open queue line with no commit in $Q: $line"
        qh=$(git -C "$REPO" rev-parse --verify --quiet "$w^{commit}") || refuse "open queue line, commit $w is not in $REPO ($Q)"
        git -C "$REPO" merge-base --is-ancestor "$qh" "$H" || refuse "open queue line $w is not in the install commit $S: install only the full set ($Q)"
    done < "$Q"
    QNOTE="; queue: $nl open lines in $S"
}
# Development evidence is separate from the release gate and never enters its index.
if [ -n "$DEV_RECORD" ]; then
    [ "$CONFIG" = dev ] || refuse "development evidence cannot authorize a release install"
    [ -n "$FWSC" ] || refuse "development validation requires --fwsc"
    DEV_ARGS=(--repo "$REPO" --commit "$H" --record "$DEV_RECORD" --fwsc "$FWSC")
    [ -z "$CTL" ] || DEV_ARGS+=(--ctl "$CTL")
    python3 "$(dirname "$0")/dev_validate.py" verify "${DEV_ARGS[@]}" || refuse "invalid development evidence"
    queue_check
    echo "DEV OK $S $DEV_RECORD (config dev$QNOTE)"
    exit 0
fi
[ -f "$INDEX" ] || refuse "no $INDEX"
crc32() { python3 -c 'import sys, zlib; print("%08x" % (zlib.crc32(open(sys.argv[1], "rb").read()) & 0xffffffff))' "$1"; }
FCRC=""; CCRC=""
if [ -n "$FWSC" ]; then [ -f "$FWSC" ] || refuse "no file $FWSC"; FCRC=$(crc32 "$FWSC") || refuse "cannot read $FWSC"; fi
if [ -n "$CTL" ]; then [ -f "$CTL" ] || refuse "no file $CTL"; CCRC=$(crc32 "$CTL") || refuse "cannot read $CTL"; fi
# 2. the rows of this tree, newest first: "result<TAB>result file"
ROWS=$(awk -F '|' -v t="$T12" '
    { tr=$3; gsub(/^ +| +$/, "", tr); if (tr != t) next
      r=$5; gsub(/^ +| +$/, "", r); f=$6; gsub(/^ +| +$/, "", f); split(f, a, " ")
      print r "\t" a[1] }' "$INDEX" | sed '1!G;h;$!d')
[ -n "$ROWS" ] || refuse "no index row for tree $T12 in $INDEX"
LAST=""; CAND=0   # LAST: the reason of the newest gate-PASS row (else of the newest row)
OLDIFS=$IFS; IFS='
'
for row in $ROWS; do
    IFS=$OLDIFS
    R=$(printf '%s' "$row" | cut -f1); F=$(printf '%s' "$row" | cut -f2)
    case "$R" in
    "PASS"|"PASS (result used again"*|"PASS (force: "*) ;;
    *) [ -z "$LAST" ] && LAST="newest row result \"$R\" is not a gate PASS"; continue ;;
    esac
    # 3. the result file
    [ "$CAND" = 0 ] && LAST=""; CAND=$((CAND + 1))
    RF="$RESULTS/$F"
    case "$F" in */*|"") [ -z "$LAST" ] && LAST="row result file \"$F\" is not a file name"; continue ;; esac
    [ -f "$RF" ] || { [ -z "$LAST" ] && LAST="row $R: no result file $RF"; continue; }
    grep -qx 'Result: PASS' "$RF" || { [ -z "$LAST" ] && LAST="$F: no line \"Result: PASS\""; continue; }
    grep -q "^- Tree (SHA-1 of git ls-tree -r without \*\.md): $TREE\$" "$RF" || { [ -z "$LAST" ] && LAST="$F: the tree is not $TREE"; continue; }
    # 4. the logs folder: gate.log by the gate, the CRC manifest
    L=$(sed -n 's/^- Worktree: .*\. Logs: \(.*\)\.$/\1/p' "$RF" | head -1)
    [ -n "$L" ] || { [ -z "$LAST" ] && LAST="$F: no logs folder"; continue; }
    if [ -n "${X0X_GATE_LOGS_MAP:-}" ]; then MF=${X0X_GATE_LOGS_MAP%%=*}; MT=${X0X_GATE_LOGS_MAP#*=}
        case "$L" in "$MF"*) L="$MT${L#"$MF"}" ;; esac; fi
    case "$L" in /*) [ -d "$L" ] || [ ! -d "$RESULTS/logs$L" ] || L="$RESULTS/logs$L" ;; esac
    [ -f "$L/gate.log" ] || { [ -z "$LAST" ] && LAST="$F: no gate log $L/gate.log"; continue; }
    grep -q '^gate: ' "$L/gate.log" || { [ -z "$LAST" ] && LAST="$F: $L/gate.log has no gate lines"; continue; }
    [ -s "$L/crc.txt" ] || { [ -z "$LAST" ] && LAST="$F: no CRC manifest $L/crc.txt"; continue; }
    grep -qx '## CRC manifest' "$RF" || { [ -z "$LAST" ] && LAST="$F: no CRC manifest section"; continue; }
    # Provenance travels with exact artifacts; the installer need not have the build SDK.
    INPUT_ID=$(sed -n 's/^- Validation inputs: //p' "$RF")
    INPUT_SOURCE=$(git -C "$REPO" ls-tree -r --full-tree "$H" | awk -F '\t' '$2 !~ /\.md$/' | python3 -c 'import hashlib,sys; print(hashlib.sha256(sys.stdin.buffer.read()).hexdigest())')
    python3 "$(dirname "$0")/validation_inputs.py" verify "$L/validation-inputs.json" \
        --sha256 "$INPUT_ID" --source "$INPUT_SOURCE" 2>/dev/null || {
        [ -z "$LAST" ] && LAST="$F: missing or invalid validation-input provenance"; continue; }
    # 5. the files to install
    MAN=$(sed -n '/^## CRC manifest/,/^## [^C]/p' "$RF")
    TWO=0; grep -q '^- Config CRCs: ' "$RF" && TWO=1
    if [ "$CONFIG" = rel ] && [ "$TWO" = 0 ]; then
        [ -z "$LAST" ] && LAST="$F: a result with one config (dev only), no rel build"; continue; fi
    if [ "$CONFIG" = rel ]; then PRE="rel/build/"; OPRE="build/"; OTHER=dev; else PRE="build/"; OPRE="rel/build/"; OTHER=rel; fi
    if [ -n "$FCRC" ] && ! echo "$MAN" | grep -qE "^$PRE[^ /]*\.fwsc crc32 $FCRC,"; then
        if [ "$TWO" = 1 ] && echo "$MAN" | grep -qE "^$OPRE[^ /]*\.fwsc crc32 $FCRC,"; then
            refuse "$(basename "$FWSC") crc32 $FCRC is the $OTHER build, not the $CONFIG build, of tree $T12 (CRC manifest of $F)"; fi
        refuse "$(basename "$FWSC") crc32 $FCRC is not a gate-built $CONFIG .fwsc of tree $T12 (CRC manifest of $F)"; fi
    if [ -n "$CCRC" ] && ! echo "$MAN" | grep -q "^ctl.img file crc32 $CCRC,"; then
        refuse "$(basename "$CTL") crc32 $CCRC is not the gate-built ctl.img of tree $T12 (CRC manifest of $F)"; fi
    queue_check
    echo "GATE OK $S $RF (config $CONFIG$QNOTE)"
    exit 0
done
IFS=$OLDIFS
refuse "no valid gate PASS for tree $T12 ($LAST; $CAND gate-PASS rows checked)"
