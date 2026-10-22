#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Pre-install gate for the SMK-37 Elite firmware (Linux x86-64, native or local container). It does not touch a device.
#   tools/gate.sh <commit or ref> --purpose <purpose> --reason "<concrete reason>" [--force "<reason>"] [--trial]
# Purpose: release-install, release-package, or owner-validation. Identify the release
# step/artifact or explicit owner request in --reason. Development uses scoped checks.
# Missing purpose/reason is refused before setup, result writes, clones or locks.
# --force also needs a separate reason why matching passing evidence cannot be reused.
# The commit must be on the integration branch of the box repository (X0X_INTEGRATION). Else the gate stops with
# "FAIL <hash> not on integration (use --trial)". With --trial the gate runs any commit, but the result is a TRIAL:
# the result file and the index row are marked TRIAL, the output line starts with TRIAL, and it never counts
# as a release PASS. A trial run never uses a result again and never writes <tree>.md.
# Results: each run writes its own file $X0X_RESULTS/<tree>-<YYYYmmdd-HHMM>.md (TRIAL: <tree>.trial-<...>.md)
# and its own index row. <tree>.md is the first PASS of the tree (the canonical result). The gate never replaces
# a PASS <tree>.md. If a PASS <tree>.md exists, the gate uses it again (no lock, no build, one index row).
# --force "<reason>" runs again; the reason is required and goes into the index row.
# Preflight (before the clone, the builds and the lock): the gate finds the JieLi toolchain and the AC79 SDK
# (X0X_TOOLCHAIN or JIELI_TOOLCHAIN, AC79_SDK; else ~/.jieli/toolchain and ~/fw-AC79_AIoT_SDK of $HOME, of the user's
# home from getent, and of /home/box). It links them into the private HOME of the run. If one is missing, it writes
# "VOID <hash> setup: ..." and exits with 2. A setup error is not a test result: no result file and no index row.
# Work folder: X0X_GATE_WORK must not be under /tmp, because the Bluetooth build mounts a private /tmp over /tmp
# and then cannot see the worktree. A work folder under /tmp is moved to ~/.cache/x0x37-gate of the user's real home.
# The gate makes its own worktree of the commit, and then it does these steps:
#   0. it checks that each reference folder is at its pinned commit (X0X_REF_PINS),
#   1. the Elite build, the Pro build (build/x0x-smk37.fwsc, for the package tests), and
#      the Bluetooth image build (private /tmp). Two build configs, each in its own worktree, at the same time:
#      dev (X0X_DEV_PADCAL=1, worktree <commit>) and rel (X0X_DEV_PADCAL=0, the release build, worktree <commit>.rel).
#      The gate sets the build flags itself: it removes X0X_DEV_* and the other build variables of the caller
#      (see BUILD_VARS), so the caller's environment never changes a build. The rel worktree gets the dev ctl.img.
#   2. tests/run_tests.sh with AC79_SDK and the reference variables (parallel if the tree has X0X_TEST_JOBS),
#      for the two configs at the same time (each with its own private HOME and TMPDIR).
#      The gate compares the tests that ran with the list of tests in the script. A missing test is a failure.
#   3. the flash-map check (tools/gate_flashmap.py, with the owner tag), the budget checks for both models,
#      the X0X_RAM_CANARY default (it must be 0) and a CRC manifest.
#      The CRC manifest has a line for build/x0x-smk37-elite-ble.fwsc (tools/add_ctl.py: the Elite package and the
#      gate's ctl.img), the BLE Elite package that is installed. If the gate cannot make it, the result is FAIL.
#      Steps 2 and 3 apply to the two configs. The result is PASS only if the two configs pass. The CRC manifest has
#      the dev lines (build/...) and the rel lines (rel/build/...). The result file has the line
#      "- Config CRCs: dev elite <crc>, ble <crc>; rel elite <crc>, ble <crc>", and the index row has it in the
#      last column (Configs). tools/require_gate_pass.sh --config dev|rel uses it.
# The builds and the tests use a private HOME and TMPDIR for each run (<worktree>.logs/home and tmp).
# The gate always uses X0X_TEST_TIME_SCALE=1 (the real waits of the updater in install-slots-web), also if the
# caller sets another value. The PASS or FAIL line and the result file give the time scale.
# The lock file is next to the read-only gate folder (/workspace/x0x37-refs/gate.lock), because the gate folder is read-only.
# Step 2 holds a box-wide lock (flock on X0X_GATE_LOCK), so the tests of two gate runs (from any agent) never
# run at the same time. While it waits, the gate writes "waiting for gate lock" to stderr. The builds and a
# result that the gate uses again do not take the lock.
# On SIGTERM, SIGINT or SIGHUP the gate stops all its builds and tests (each runs in its own process group),
# releases the lock, writes "KILLED <commit> <signal> (no result)", writes no result file and exits with 143, 130 or 129.
# No child gets the lock file descriptor, so a child that stays alive cannot keep the lock.
# Sender of a stop signal: the gate runs itself through tools/gate_sigwrap.py (exec, the
# same PID), when that file is beside the gate. The wrapper reads the PID, working folder and command line of the process
# that sent the signal. The KILLED lines then name them: gate.log "gate: KILLED by SIGTERM from pid N (cwd C; cmd X)
# (no result)" and the output "KILLED <commit> SIGTERM from pid N cwd C (no result)". No wrapper (or a signal sent to the
# inner gate process): "sender unknown". X0X_GATE_INNER=1 runs the gate with no wrapper.
# Script change: the gate keeps the mtime of its own script and of gate_flashmap.py at the
# start. If one changes during the run, gate.log gets "gate: SCRIPT CHANGED during the run: <file> <old> -> <new>" (at
# the end of the run and on a stop signal), and stderr gets the same line. The result does not change.
# The install-path tests in run_tests.sh take a shared lock (flock -s) on the same file, so the gate waits for them.
# While the gate holds its exclusive lock it sets X0X_GATE_HOLDS_LOCK=1, and its own tests do not take the shared lock.
# The tree is a SHA-1 of "git ls-tree -r" without the *.md files.
# Output: one line, "PASS <hash>" or "FAIL <hash> <reason>" ("TRIAL PASS" or "TRIAL FAIL" with --trial), or
# "VOID <hash> setup: ..." (exit code 2). Exit code 0 or 1. The full log is in the worktree folder.
# Environment (defaults in brackets):
#   X0X_REPO [/workspace/x0x37.git]  X0X_RESULTS [/workspace/x0x37-results]  X0X_REFS [/workspace/x0x37-refs/schwung]
#   X0X_GATE_WORK [~/.cache/x0x37-gate, never under /tmp]  X0X_TEST_JOBS [8]  X0X_TOOLCHAIN or JIELI_TOOLCHAIN, AC79_SDK [see Preflight]
#   X0X_PREPUSH_HOOK [/workspace/x0x37.git/hooks/pre-push]: the gate copies it into its .repo clone. A failed copy is a FAIL.
#   X0X_GATE_SETUP_ONLY [0]: 1 = make the .repo clone (hook, push URLs), print "SETUP <commit> <path>" and stop (a test of this step).
#   X0X_GATE_LOCK [/workspace/x0x37-refs/gate.lock]
#   X0X_REF_PINS ["schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d"]
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE   # a hook sets these; a scratch repository must not use the repository of the hook
ORIGINAL_ARGS=("$@")
FORCE=0; TRIAL=0; ARG=""; FREASON=""; PURPOSE=""; RUN_REASON=""
while [ $# -gt 0 ]; do
    case "$1" in
    --force) FORCE=1; FREASON=${2:-}; [ $# -ge 2 ] && shift
             case "$FREASON" in ""|-*|[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]) FREASON="" ;; esac
             [ -n "$FREASON" ] || { echo "REFUSED: --force needs a rerun reason in addition to --purpose and --reason" >&2; exit 2; } ;;
    --purpose|--reason)
        opt=$1
        [ $# -ge 2 ] && [[ "$2" != -* ]] || { echo "REFUSED: $opt needs a value" >&2; exit 2; }
        if [ "$opt" = --purpose ]; then PURPOSE=$2; else RUN_REASON=$2; fi
        shift ;;
    --trial) TRIAL=1 ;;
    -*) echo "unknown option $1" >&2; exit 2 ;;
    *) [ -z "$ARG" ] || { echo "REFUSED: give exactly one commit" >&2; exit 2; }; ARG=$1 ;;
    esac
    shift
done
set -- "$ARG"
[ -n "${1:-}" ] || { echo "usage: tools/gate.sh <commit or ref> --purpose <purpose> --reason \"<concrete reason>\" [--force \"<reason>\"] [--trial]" >&2; exit 2; }
FREASON=$(printf '%s' "$FREASON" | tr '|\n' '/ ' | cut -c1-120)

case "$PURPOSE" in
    release-install|release-package|owner-validation) ;;
    *) echo "REFUSED: --purpose must be release-install, release-package, or owner-validation; use scoped checks for development" >&2; exit 2 ;;
esac
# A reason is required even for trial, force, cache reuse and setup-only calls.
# This records the caller's justification; it cannot verify owner intent.
[[ "$RUN_REASON" =~ [[:alnum:]] ]] || { echo "REFUSED: --reason must identify the release step or owner request" >&2; exit 2; }
if [ "$TRIAL" = 1 ] && [ "$PURPOSE" != owner-validation ]; then
    echo "REFUSED: --trial requires --purpose owner-validation; trial results cannot qualify a release" >&2; exit 2
fi
[[ "$FREASON" =~ [[:alnum:]] ]] || { [ "$FORCE" = 0 ] || { echo "REFUSED: --force needs a concrete rerun reason" >&2; exit 2; }; }
RUN_REASON=$(printf '%s' "$RUN_REASON" | tr '|\r\n' '/  ')

GATE_DIR=$(cd "$(dirname "$0")" && pwd)
if [ "$(uname -s)" != Linux ]; then
    echo "VOID ${1:-unknown} setup: use Linux x86-64 or the local Linux container (BUILDING.md)"; exit 2
fi
if [ -z "${X0X_GATE_INNER:-}" ] && [ -f "$GATE_DIR/gate_sigwrap.py" ] && command -v python3 >/dev/null 2>&1; then
    X0X_GATE_SIGFILE=$(mktemp "${TMPDIR:-/tmp}/x0x37-gate-sig.XXXXXX") && export X0X_GATE_INNER=1 X0X_GATE_SIGFILE &&
        exec python3 "$GATE_DIR/gate_sigwrap.py" "$X0X_GATE_SIGFILE" bash "$GATE_DIR/$(basename "$0")" "${ORIGINAL_ARGS[@]}"
fi
SELF_FILES="$GATE_DIR/$(basename "$0") $GATE_DIR/gate_flashmap.py"
SELF_M0=$(for f in $SELF_FILES; do stat -c '%Y' "$f" 2>/dev/null || echo -; done | tr '\n' ' ')
script_check() {   # check 13: a gate script changed during the run -> a line in gate.log (if there is one) and stderr
    local f m0 m1 i=0 msg
    for f in $SELF_FILES; do
        i=$((i + 1)); m0=$(echo "$SELF_M0" | awk -v i=$i '{print $i}'); m1=$(stat -c '%Y' "$f" 2>/dev/null || echo -)
        [ "$m0" = "$m1" ] && continue
        msg="gate: SCRIPT CHANGED during the run: $f $(date -d "@$m0" '+%H:%M:%S' 2>/dev/null || echo "$m0") -> $(date -d "@$m1" '+%H:%M:%S' 2>/dev/null || echo "$m1")"
        [ -n "${LOG:-}" ] && [ -d "$(dirname "$LOG")" ] && echo "$msg" >> "$LOG"
        echo "$msg" >&2
    done
}
sender() {         # check 5: "from pid N (cwd C; cmd X)" from the wrapper file, or "sender unknown"
    local i=0 l=""
    while [ -n "${X0X_GATE_SIGFILE:-}" ] && [ $i -lt 10 ]; do
        [ -s "$X0X_GATE_SIGFILE" ] && { l=$(head -1 "$X0X_GATE_SIGFILE"); break; }
        sleep 0.1; i=$((i + 1))
    done
    if [ -n "$l" ]; then
        SND_PID=$(echo "$l" | sed -n 's/.* pid=\([0-9]*\) .*/\1/p'); SND_CWD=$(echo "$l" | sed -n 's/.* cwd=\(.*\) cmd=.*/\1/p')
        SND_CMD=$(echo "$l" | sed -n 's/.* cmd=\(.*\)$/\1/p')
        SND_LOG="from pid $SND_PID (cwd $SND_CWD; cmd $SND_CMD)"; SND_OUT="from pid $SND_PID cwd $SND_CWD"
    else SND_LOG="sender unknown"; SND_OUT="sender unknown"; fi
}
REPO=${X0X_REPO:-$(git -C "$GATE_DIR/.." rev-parse --show-toplevel 2>/dev/null || echo /workspace/x0x37.git)}
RESULTS=${X0X_RESULTS:-/workspace/x0x37-results}
REFS=${X0X_REFS:-/workspace/x0x37-refs/schwung}
WORK=${X0X_GATE_WORK:-${RHOME:-$HOME}/.cache/x0x37-gate}
JOBS=${X0X_TEST_JOBS:-8}
RHOME=$(getent passwd "$(id -un)" | cut -d: -f6)       # the real home of the user (HOME can be a private folder)
pick() {                # the first candidate that has the test file $1; else the first candidate (for the message)
    local t=$1; shift; local c
    for c in "$@"; do [ -n "$c" ] && [ -e "$c/$t" ] && { echo "$c"; return; }; done; echo "${1:-}"
}
TCF=pi32v2/bin/clang; SDKF=cpu/wl82/tools/uboot.boot          # the files that build.sh checks
if [ -n "${X0X_TOOLCHAIN:-}${JIELI_TOOLCHAIN:-}" ]; then TC=${X0X_TOOLCHAIN:-$JIELI_TOOLCHAIN}
else TC=$(pick $TCF "$HOME/.jieli/toolchain" "$RHOME/.jieli/toolchain" /home/box/.jieli/toolchain); fi
if [ -n "${AC79_SDK:-}" ]; then SDK=$AC79_SDK
else SDK=$(pick $SDKF "$HOME/fw-AC79_AIoT_SDK" "$RHOME/fw-AC79_AIoT_SDK" /home/box/fw-AC79_AIoT_SDK); fi
SCALE=1                                 # the pre-install gate: real updater waits only
export X0X_TEST_TIME_SCALE=$SCALE
# The build variables of the caller never go to a build: the gate sets X0X_DEV_PADCAL for each config itself.
BUILD_VARS="X0X_CDC X0X_DEBUG X0X_BREAK_BANK X0X_RAM_CANARY X0X_SMK_LCD_BAUD X0X_FONTSET X0X_FONT_DIRS FELUCCA_DATA_FAMILY FELUCCA_DATA_UNTAGGED_OK BOARD MODEL JIELI_DOCKER JIELI_DOCKER_IMAGE"
UNSET_TXT=""
for v in $(compgen -e | grep -E '^X0X_DEV_') $BUILD_VARS; do
    [ -n "${!v+x}" ] && UNSET_TXT="$UNSET_TXT $v=${!v}"
    unset "$v"
done
PREPUSH=${X0X_PREPUSH_HOOK:-$GATE_DIR/pre-push.hook}
GLOCK=${X0X_GATE_LOCK:-/workspace/x0x37-refs/gate.lock}
PINS=${X0X_REF_PINS:-schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d}
INTB=${X0X_INTEGRATION:-integration}

H=$(git -C "$REPO" rev-parse --verify --quiet "$1^{commit}") || { echo "FAIL $1 unknown commit in $REPO"; exit 1; }
S=${H:0:7}
if [ "$TRIAL" = 0 ] && ! git -C "$REPO" merge-base --is-ancestor "$H" "refs/heads/$INTB" 2>/dev/null; then
    echo "FAIL $S not on $INTB (use --trial)"; exit 1
fi
TP=""; [ "$TRIAL" = 1 ] && TP="TRIAL "     # the prefix of the output line and of the result
# Stop on a signal: kill the process group of the current child, release the lock, write no result.
CHILD=""; FLPID=""
exec 7>&1                       # the gate stdout: the KILLED line goes here, also while a child writes to a log
on_signal() {
    trap '' TERM INT HUP
    [ -n "$FLPID" ] && kill -TERM "$FLPID" 2>/dev/null
    if [ -n "$CHILD" ]; then
        kill -TERM -- "-$CHILD" 2>/dev/null
        for _ in 1 2 3 4 5 6 7 8 9 10; do kill -0 -- "-$CHILD" 2>/dev/null || break; sleep 0.5; done
        kill -KILL -- "-$CHILD" 2>/dev/null
    fi
    { flock -u 9; exec 9>&-; } 2>/dev/null
    rm -f "${TMPF:-}" 2>/dev/null
    sender; script_check
    [ -n "${LOG:-}" ] && [ -d "$(dirname "$LOG")" ] && echo "gate: KILLED by $1 $SND_LOG (no result)" >> "$LOG"
    echo "KILLED $S $1 $SND_OUT (no result)" >&7
    exit "$2"
}
trap 'on_signal SIGTERM 143' TERM
trap 'on_signal SIGINT 130' INT
trap 'on_signal SIGHUP 129' HUP
# Run a child in its own process group (setsid), and wait for it so that a signal stops the gate at once.
# The caller closes fd 9 for the child (9>&-), so the child never holds the gate lock.
child() { setsid "$@" 7>&- & CHILD=$!; wait "$CHILD"; local rc=$?; CHILD=""; return $rc; }
TREE=$(git -C "$REPO" ls-tree -r --full-tree "$H" | awk -F '\t' '$2 !~ /\.md$/' | sha1sum | cut -c1-40)
GTREE=$(git -C "$REPO" rev-parse "$H^{tree}")
# Capture actual inputs before looking for a reusable PASS. Missing inputs cannot match.
input_identity() {
    python3 "$GATE_DIR/validation_inputs.py" capture --repo "$REPO" --commit "$H" \
        --toolchain "$TC" --sdk "$SDK" --refs "$REFS" --pins "$PINS" --hook "$PREPUSH" --runner "$GATE_DIR/$(basename "$0")"
}
INPUT_JSON=$(input_identity) || { echo "VOID $S setup: cannot identify validation inputs"; exit 2; }
INPUT_SOURCE=$(printf '%s' "$INPUT_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["inputs"]["source"])')
INPUT_ID=$(printf '%s' "$INPUT_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["sha256"])')
NOW=$(date '+%Y-%m-%d %H:%M %Z')
mkdir -p "$RESULTS" && chmod a+rx "$RESULTS"
INDEX="$RESULTS/index.md"
INDEX_NOTE='Each line gives the commit, the tree, the git tree, the result and the time. The key is the tree: the SHA-1 of "git ls-tree -r --full-tree" without the *.md files. The git tree is "git rev-parse <commit>^{tree}". Older rows (before the Git tree column) have no git tree. In some older rows, the Tree column has the git ^{tree} and the text "(git ^{tree})". The Configs column (newer rows) gives the CRCs, the code free and the pool headroom of the dev (X0X_DEV_PADCAL=1) and the rel (X0X_DEV_PADCAL=0) Elite builds.'
[ -f "$INDEX" ] || printf '# x0x37 gate results: index\n\n%s\n\n| Commit | Tree | Git tree | Result | Result file | Time | Configs |\n|---|---|---|---|---|---|---|\n' "$INDEX_NOTE" > "$INDEX"
if ! grep -q '| Git tree |' "$INDEX"; then      # an index from an older gate: add the Git tree column to the header
    python3 - "$INDEX" "$INDEX_NOTE" <<'PY' || { echo "FAIL $1 cannot update $INDEX"; exit 1; }
import sys
p, note = sys.argv[1], sys.argv[2]
out = []
for ln in open(p).read().split("\n"):
    if ln.startswith("Each line gives the commit"): ln = note
    elif ln.startswith("| Commit | Tree | Result"): ln = "| Commit | Tree | Git tree | Result | Result file | Time |"
    elif ln.startswith("|---|---|---|---|---|") and ln.count("|") == 6: ln = "|---|---|---|---|---|---|"
    elif ln.startswith("| ") and ln.count(" | ") == 4:   # an older row: put an empty Git tree cell after the Tree cell
        c = ln.split(" | ")
        ln = " | ".join(c[:2] + ["-"] + c[2:])
    out.append(ln)
open(p + ".tmp", "w").write("\n".join(out)); __import__("os").replace(p + ".tmp", p)
PY
fi
if ! grep -q '| Configs |' "$INDEX"; then     # add the Configs column to the header (older rows have no Configs cell)
    sed -i -e 's/^| Commit | Tree | Git tree | Result | Result file | Time |$/| Commit | Tree | Git tree | Result | Result file | Time | Configs |/' \
        -e 's/^|---|---|---|---|---|---|$/|---|---|---|---|---|---|---|/' "$INDEX" || { echo "FAIL $S cannot update $INDEX"; exit 1; }
fi
chmod a+r "$INDEX"
CANON="$RESULTS/$TREE.md"                # the first PASS of the tree; the gate never replaces a PASS here
CANON_R=""; [ -f "$CANON" ] && CANON_R=$(sed -n 's/^Result: \([A-Z]*\).*/\1/p' "$CANON" | head -1)
STAMP=$(date '+%Y%m%d-%H%M')
RF="$RESULTS/$TREE-$STAMP.md"; [ "$TRIAL" = 1 ] && RF="$RESULTS/$TREE.trial-$STAMP.md"
[ -e "$RF" ] && RF="${RF%.md}-$$.md"      # two runs in one minute
# Reuse requires both configs and the exact current validation-input identity.
# An older PASS (one config, dev only) does not count: the gate runs again (the canonical <tree>.md stays).
candidate_inputs_match() {
    local logs
    logs=$(sed -n 's/^- Worktree: .*\. Logs: \(.*\)\.$/\1/p' "$1" | head -1)
    [ -n "$logs" ] && python3 "$GATE_DIR/validation_inputs.py" verify "$logs/validation-inputs.json" \
        --sha256 "$INPUT_ID" --source "$INPUT_SOURCE" 2>/dev/null
}
REUSE=""
for f in "$CANON" $(ls -t "$RESULTS/$TREE"-[0-9]*.md 2>/dev/null); do
    [ -f "$f" ] && grep -qx 'Result: PASS' "$f" && grep -q '^- Config CRCs: ' "$f" && grep -qx -- "- Validation inputs: $INPUT_ID" "$f" && candidate_inputs_match "$f" && { REUSE=$f; break; }
done
if [ "$FORCE" = 0 ] && [ "$TRIAL" = 0 ] && [ -n "$REUSE" ]; then
    CFG=$(sed -n 's/^- Config CRCs: //p' "$REUSE" | head -1)
    echo "| $S | ${TREE:0:12} | ${GTREE:0:12} | PASS (result used again) | $(basename "$REUSE") | $NOW | $CFG; purpose: $PURPOSE; reason: $RUN_REASON |" >> "$INDEX"
    OSC=$(sed -n 's/^- Time scale (X0X_TEST_TIME_SCALE): \([0-9]*\).*/\1/p' "$REUSE" | head -1)
    echo "PASS $S (time scale ${OSC:-1}) (result used again: tree ${TREE:0:12} has a PASS of the two configs)"; exit 0
fi

# Preflight: the toolchain and the SDK. A setup error is not a test result: no result file, no index row.
[ -x "$TC/$TCF" ] || { echo "VOID $S setup: no toolchain at $TC (no $TCF)"; exit 2; }
[ -f "$SDK/$SDKF" ] || { echo "VOID $S setup: no AC79 SDK at $SDK (no $SDKF)"; exit 2; }
case "$(realpath -m "$WORK")/" in
/tmp/*) NW="${RHOME:-/home/box}/.cache/x0x37-gate"
        echo "gate: the work folder $WORK is under /tmp (the Bluetooth build cannot see it); the gate uses $NW" >&2
        WORK=$NW ;;
esac

mkdir -p "$WORK"
[ -d "$WORK/.repo" ] || git clone -q --no-local --no-checkout "$REPO" "$WORK/.repo" || { echo "FAIL $S git clone of $REPO failed"; exit 1; }
# The clone gets the pre-push hook of the box repository, and no remote can push outside the box.
# The gate does this for a new clone and also for an older clone without the hook.
HC=$(git -C "$WORK/.repo" rev-parse --path-format=absolute --git-common-dir) && mkdir -p "$HC/hooks" &&
    cp "$PREPUSH" "$HC/hooks/pre-push" && chmod +x "$HC/hooks/pre-push" ||
    { echo "FAIL $S cannot copy the pre-push hook $PREPUSH into $WORK/.repo"; exit 1; }
for rem in $(git -C "$WORK/.repo" remote); do
    pu=$(git -C "$WORK/.repo" remote get-url --push "$rem")
    [ "$pu" = /workspace/x0x37.git ] || git -C "$WORK/.repo" remote set-url --push "$rem" DISABLED-no-push-outside-box ||
        { echo "FAIL $S cannot disable the push URL of remote $rem in $WORK/.repo"; exit 1; }
done
if [ "${X0X_GATE_SETUP_ONLY:-0}" = 1 ]; then echo "SETUP $S $WORK/.repo (clone, hook and push URLs only)"; exit 0; fi
FE=$(git -C "$WORK/.repo" fetch -q --update-shallow origin '+refs/heads/*:refs/remotes/origin/*' 2>&1) || {
    echo "FAIL $S git fetch from $REPO: $(echo "$FE" | grep -v '^$' | tail -n1)"; exit 1; }
if ! git -C "$WORK/.repo" cat-file -e "$H^{commit}" 2>/dev/null; then
    FE=$(git -C "$WORK/.repo" fetch -q origin "$H" 2>&1) || { echo "FAIL $S git fetch $H: $(echo "$FE" | grep -v '^$' | tail -n1)"; exit 1; }
fi
W="$WORK/$S"
LOCK="$WORK/.lock-$S"
mkdir "$LOCK" 2>/dev/null || { echo "FAIL $S another gate run uses $W"; exit 1; }
trap 'rmdir "$LOCK" 2>/dev/null' EXIT
WR="$W.rel"                              # the worktree of the rel config (X0X_DEV_PADCAL=0)
git -C "$WORK/.repo" worktree prune
for w in "$W" "$WR"; do
    if [ -d "$w" ]; then git -C "$w" checkout -q --detach "$H" && git -C "$w" reset -q --hard "$H"
    else git -C "$WORK/.repo" worktree add -q --detach "$w" "$H"; fi || { echo "FAIL $S cannot make the worktree $w"; exit 1; }
done
L="$WORK/$S-$STAMP-$$.logs"; mkdir -p "$L"  # preserve evidence for every prior run
LOG="$L/gate.log"
printf '%s\n' "$INPUT_JSON" > "$L/validation-inputs.json"
printf 'gate: Purpose: %s; Run reason: %s\n' "$PURPOSE" "$RUN_REASON" >> "$LOG"
T0=$(date +%s)
REASON=""
fail() { [ -n "$REASON" ] || REASON="$1"; echo "gate: FAIL $1" >> "$LOG"; }

cd "$W" || exit 1
PH="$L/home"; PT="$L/tmp"; mkdir -p "$PH" "$PT"
export HOME="$PH" TMPDIR="$PT"          # a private HOME and TMPDIR for all builds and tests of this run
PHR="$L/home-rel"; PTR="$L/tmp-rel"; mkdir -p "$PHR" "$PTR"   # the private HOME and TMPDIR of the rel config
for h in "$PH" "$PHR"; do mkdir -p "$h/.jieli"; ln -sfn "$TC" "$h/.jieli/toolchain"; ln -sfn "$SDK" "$h/fw-AC79_AIoT_SDK"; done   # the real toolchain and SDK
[ -n "$UNSET_TXT" ] && echo "gate: removed the build variables of the caller:$UNSET_TXT" >> "$LOG"
for d in "$REFS"/schwung-*; do [ -d "$d" ] || fail "no reference sources in $REFS"; done
PINLOG=""
for p in $PINS; do                      # each reference folder must be at its pinned commit
    n=${p%%=*}; want=${p#*=}
    got=$(git -C "$REFS/$n" rev-parse HEAD 2>/dev/null) || { fail "reference $n: not a git folder in $REFS"; PINLOG="$PINLOG $n=?"; continue; }
    case "$got" in "$want"*) ;; *) fail "reference $n is at ${got:0:7}, pinned ${want}" ;; esac
    PINLOG="$PINLOG $n=${got:0:7}"
done
ls -d "$WORK"/schwung-* >/dev/null 2>&1 && fail "../schwung-* exists next to the worktree"
# Fallback: an older tests/host/run_drum909.sh copies the 9W9 sources with their read-only modes and then
# writes into the copy. For such a tree, use a writable copy of the references for this run only.
if [ -f tests/host/run_drum909.sh ] && ! grep -q 'chmod -R u+w "$OUT/ref_src"' tests/host/run_drum909.sh; then
    rm -rf "$L/refs"; mkdir -p "$L/refs"
    cp -r --no-preserve=mode "$REFS"/schwung-* "$L/refs/" && chmod -R u+w "$L/refs" && REFS="$L/refs"
    echo "gate: the tree has the old run_drum909.sh; the tests use the writable copy $REFS" >> "$LOG"
fi
export AC79_SDK="$SDK" JIELI_TOOLCHAIN="$TC"
export NINEW9_DIR="$REFS/schwung-9W9" EIGHTW8_DIR="$REFS/schwung-8W8" SCHWUNG303="$REFS/schwung-303"
export TB3PO_REF="$REFS/schwung-tb3po/src/dsp/tb3po.c"

# 1. builds
# build_config <worktree> <home> <tmp> <flag> <log suffix>: the Elite and Pro builds of one config.
# The exit code of each build goes to $L/build-<b><suffix>.rc.
build_config() {
    cd "$1" || return 1
    export HOME="$2" TMPDIR="$3" X0X_DEV_PADCAL="$4"
    MODEL=elite ./build.sh --board smk37 > "$L/build-elite$5.log" 2>&1; echo $? > "$L/build-elite$5.rc"
    ./build.sh --board smk37 > "$L/build-pro$5.log" 2>&1; echo $? > "$L/build-pro$5.rc"
}
export -f build_config; export L
B0=$(date +%s)
child bash -c 'build_config "$1" "$2" "$3" 1 "" & build_config "$4" "$5" "$6" 0 -rel & wait' _ "$W" "$PH" "$PT" "$WR" "$PHR" "$PTR" 9>&-
echo "gate: builds of the two configs took $(( $(date +%s) - B0 )) s" >> "$LOG"
for c in "dev:" "rel:-rel"; do
    cn=${c%%:*}; sx=${c#*:}
    for b in "elite:Elite" "pro:Pro"; do
        f="$L/build-${b%%:*}$sx.log"
        [ "$(cat "$L/build-${b%%:*}$sx.rc" 2>/dev/null)" = 0 ] || fail "${b#*:} build ($cn): $(grep -m1 -iE 'error|fail' "$f")"
    done
done
PCT="$L/ctl-tmp"; mkdir -p "$PCT"
child unshare -rm sh -c "mount --bind '$PCT' /tmp && env HOME='$PH' JIELI_TOOLCHAIN='$TC' SDK_LOCAL='$SDK' CTL_NATIVE=1 sh ble/ctl/build_ctl.sh" \
    > "$L/build-ctl.log" 2>&1 || fail "ctl build: $(grep -m1 -iE 'error|fail|undefined' "$L/build-ctl.log")"
mkdir -p "$WR/ble/ctl/out" && cp -a "$W/ble/ctl/out/." "$WR/ble/ctl/out/" || fail "cannot copy ctl.img into the rel worktree"

# 2. tests
MODE="X0X_TEST_JOBS=$JOBS"
grep -q X0X_TEST_JOBS tests/run_tests.sh || MODE="serial: this tree has no parallel mode"
# The box-wide gate lock: one gate test phase at a time on this box. The lock file is writable by all users.
[ -e "$GLOCK" ] || { ( umask 000; : >> "$GLOCK" ) && chmod a+rw "$GLOCK"; } 2>/dev/null
LW0=$(date +%s)
if ! exec 9>>"$GLOCK"; then
    fail "cannot open the gate lock $GLOCK"; TRC=1; TRCR=1; LOCK_TXT="not taken (cannot open $GLOCK)"
else
    if ! flock -n 9; then
        echo "gate $S: waiting for gate lock $GLOCK" >&2
        echo "gate: waiting for gate lock $GLOCK" >> "$LOG"
        flock 9 &               # in the background, so that a signal stops the wait at once
        FLPID=$!; wait "$FLPID"; FLRC=$?; FLPID=""
        [ "$FLRC" = 0 ] || { fail "cannot take the gate lock $GLOCK"; }
    fi
    LW1=$(date +%s)
    echo "gate: took the gate lock after $((LW1 - LW0)) s" >> "$LOG"
    # the two configs at the same time; the tests do not take the shared lock (X0X_GATE_HOLDS_LOCK=1)
    X0X_GATE_HOLDS_LOCK=1 X0X_TEST_JOBS="$JOBS" child bash -c '
        (cd "$1" && HOME="$2" TMPDIR="$3" X0X_DEV_PADCAL=1 sh tests/run_tests.sh > "$L/tests.log" 2>&1; echo $? > "$L/tests.rc") &
        (cd "$4" && HOME="$5" TMPDIR="$6" X0X_DEV_PADCAL=0 sh tests/run_tests.sh > "$L/tests-rel.log" 2>&1; echo $? > "$L/tests-rel.rc") &
        wait' _ "$W" "$PH" "$PT" "$WR" "$PHR" "$PTR" 9>&-
    TRC=$(cat "$L/tests.rc" 2>/dev/null || echo 1); TRCR=$(cat "$L/tests-rel.rc" 2>/dev/null || echo 1)
    flock -u 9; exec 9>&-
    LW2=$(date +%s)
    echo "gate: released the gate lock; the tests took $((LW2 - LW1)) s" >> "$LOG"
    LOCK_TXT="$GLOCK: waited $((LW1 - LW0)) s, held $((LW2 - LW1)) s"
fi
# check_tests <worktree> <tests log> <rc> <config name> <suffix>: the counts, the missing tests and the package tests of one config
check_tests() {
    local w=$1 tl=$2 rc=$3 cn=$4 sx=$5
    local ok nf ns miss
    ok=$(grep -c '^  ok ' "$tl"); nf=$(grep -c '^  FAIL' "$tl"); ns=$(grep -c '^  skip' "$tl")
    [ "$rc" = 0 ] || fail "tests ($cn): $nf failed ($(grep -m1 '^  FAIL' "$tl" | sed 's/^  FAIL //'))"
    [ "$ns" = 0 ] || fail "tests ($cn): $ns skipped"
    (cd "$w" && expected_tests "$tl" "$L/tests-expected$sx.txt" "$L/tests-missing$sx.txt")
    miss=$(cat "$L/tests-missing$sx.txt")
    [ -z "$miss" ] || fail "tests ($cn) did not run: $miss"
    for t in smk-build-package ble-package; do grep -q "^  ok   $t\$" "$tl" || fail "test $t ($cn) did not pass"; done
    C_OK=$ok; C_NF=$nf; C_NS=$ns; C_NEXP=$(grep -c . "$L/tests-expected$sx.txt"); C_MISS=$miss   # no subshell: fail() must set REASON
}
# expected_tests <tests log> <expected file> <missing file>: the tests that the script knows (in the current folder)
# A test that does not run (and does not write a "skip" line) is a failure.
expected_tests() {
python3 - "$1" "$2" <<'PY' > "$3"
import glob, os, re, sys
src = [l for l in open("tests/run_tests.sh").read().split("\n") if not l.lstrip().startswith("#")]
names = set()
for l in src:
    # Ignore inline comments while preserving quoted shell arguments.
    l = re.sub(r"('(?:[^']*)'|\"(?:\\.|[^\"\\])*\"|\\.)|#.*",
               lambda m: m.group(1) or "", l)
    for n in re.findall(r'(?:^|[\s;&|])run "?([A-Za-z0-9_.-]+)"?\s', l + " "):
        names.add(n)
    m = re.match(r'\s*for t in ([^;]+); do', l)
    if m:
        names |= {t for t in m.group(1).split() if os.path.exists(f"tests/host/run_{t}.sh")}
    m = re.match(r'\s*for m in ([^;]+); do', l)
    if m:
        names |= {f"smk-ota-entry-model{x}" for x in m.group(1).split()}
names |= {"scenario-" + os.path.basename(s)[:-4] for s in glob.glob("tests/scenarios/*.x0x")}
if not os.path.exists("build/x0x-smk37-mke.fwsc"): names.discard("smk-build-package-mke")   # the gate does not build MKE
ran = set(re.findall(r"^  (?:ok|FAIL|skip) +(\S+)", open(sys.argv[1]).read(), re.M))
open(sys.argv[2], "w").write("\n".join(sorted(names)) + "\n")
print(" ".join(sorted(names - ran)))
PY
}
check_tests "$W" "$L/tests.log" "$TRC" dev ""; OK=$C_OK; NF=$C_NF; NS=$C_NS; NEXP=$C_NEXP; MISS=$C_MISS
check_tests "$WR" "$L/tests-rel.log" "$TRCR" rel -rel; OKR=$C_OK; NFR=$C_NF; NSR=$C_NS; NEXPR=$C_NEXP; MISSR=$C_MISS
BLE=$(tail -n1 build/host/ble.log 2>/dev/null)
CK=$(tail -n1 build/host/smk-clip-keys.log 2>/dev/null)
SCEN=$(grep -c '^  ok   scenario-' "$L/tests.log"); SCENR=$(grep -c '^  ok   scenario-' "$L/tests-rel.log")

# 3. flash map, budgets, CRCs
python3 "$GATE_DIR/gate_flashmap.py" firmware/hal/fm1_flash_map.h firmware/src/storage.c > "$L/flashmap.log" 2>&1 || fail "$(tail -n1 "$L/flashmap.log")"
FM=$(tail -n1 "$L/flashmap.log")
num() { grep -m1 -oE "$1" "$2" | grep -oE '[0-9]+' | sed -n "${3}p"; }
SIZES=""
for b in elite pro elite-rel pro-rel; do   # code free, RAM free and pool headroom for each build of the two configs
    f="$L/build-$b.log"
    cf=$(num 'app code area [0-9]+ B, [0-9]+ B free' "$f" 2)
    ru=$(num 'RAM \.data\+\.bss [0-9]+ B of [0-9]+' "$f" 1); rm=$(num 'RAM \.data\+\.bss [0-9]+ B of [0-9]+' "$f" 2)
    pu=$(num 'pool [0-9]+ B of [0-9]+' "$f" 1); pm=$(num 'pool [0-9]+ B of [0-9]+' "$f" 2)
    rf=$(( ${rm:-0} - ${ru:-0} )); ph=$(( ${pm:-0} - ${pu:-0} ))
    [ -n "$cf" ] && [ "$cf" -ge 0 ] || fail "$b build: app code over the limit (no 'B free' line)"
    [ -n "$ru" ] && [ -n "$rm" ] && [ "$rf" -ge 0 ] || fail "$b build: app RAM over the limit"
    [ -n "$pu" ] && [ "$ph" -ge 8192 ] || fail "$b build: pool headroom $ph B < 8192 B"
    SIZES="$SIZES| ${b/-rel/ rel} | ${cf:-?} B | ${ru:-?} of ${rm:-?} B, $rf B free | ${pu:-?} of ${pm:-?} B, $ph B headroom |
"
    [ "$b" = elite ] && { POOL_H=$ph; CODE_FREE=$cf; RAM_USED=$ru; RAM_MAX=$rm; POOL=$pu; POOL_MAX=$pm; }
    [ "$b" = elite-rel ] && { CODE_FREE_R=$cf; POOL_H_R=$ph; }
done
# X0X_RAM_CANARY: the default in firmware/src/ble.c must be 0, and no build may set it.
CANARY=$(sed -n 's/^#define X0X_RAM_CANARY \([0-9][0-9]*\).*/\1/p' firmware/src/ble.c 2>/dev/null | head -1)
if [ -z "$CANARY" ]; then CANARY_TXT="not in this tree"
else
    CANARY_TXT="$CANARY (default in firmware/src/ble.c)"
    [ "$CANARY" = 0 ] || fail "X0X_RAM_CANARY default is $CANARY, expected 0"
fi
CSET=$(grep -lE 'X0X_RAM_CANARY=[1-9]' build.sh tools/*.py "$L"/build-*.log 2>/dev/null | tr '\n' ' ')
[ -z "$CSET" ] || { fail "X0X_RAM_CANARY is set to a value that is not 0 in: $CSET"; CANARY_TXT="$CANARY_TXT; set in $CSET"; }
CTL_LINE=$(grep -m1 'ctl.img: ' "$L/build-ctl.log")
CTL_SIZE=$(echo "$CTL_LINE" | grep -oE ': [0-9]+ B of [0-9]+' | grep -oE '[0-9]+' | head -1)
CORE=$(echo "$CTL_LINE" | grep -oE '\([0-9]+ B of [0-9]+\)' | head -1 | grep -oE '[0-9]+')
CORE_USED=$(echo "$CORE" | sed -n 1p); CORE_MAX=$(echo "$CORE" | sed -n 2p)
CORE_FREE=$(( ${CORE_MAX:-0} - ${CORE_USED:-0} ))
[ -n "$CTL_SIZE" ] && [ "$CTL_SIZE" -le 65536 ] || fail "ctl.img is not one piece of 65536 B or less"
[ -n "$CORE_USED" ] && [ "$CORE_FREE" -ge 0 ] || fail "core RAM does not fit"
# The BLE Elite package (the file that Firmware Main Builder installs): the Elite package with the gate's ctl.img added.
# One for each config, in its own worktree.
BLEPKG=build/x0x-smk37-elite-ble.fwsc
for c in "dev:$W:" "rel:$WR:-rel"; do
    cn=${c%%:*}; cw=${c#*:}; cw=${cw%:*}; sx=${c##*:}
    rm -f "$cw/$BLEPKG"
    if [ ! -f "$cw/tools/add_ctl.py" ]; then fail "BLE Elite package ($cn): no tools/add_ctl.py"
    elif ! (cd "$cw" && python3 tools/add_ctl.py build/x0x-smk37-elite.fwsc ble/ctl/out/ctl.img -o "$BLEPKG") > "$L/ble-package$sx.log" 2>&1 || [ ! -s "$cw/$BLEPKG" ]; then
        fail "BLE Elite package ($cn): add_ctl.py failed ($(tail -n1 "$L/ble-package$sx.log"))"
    fi
done
# The CRC manifest: the dev lines (build/...), then the rel lines (rel/build/...). The ctl.img is the same for the two.
python3 - "$L/crc.txt" "$WR" <<'PY' || fail "CRC manifest"
import glob, struct, sys, zlib
lines = []
def crc(p): return format(zlib.crc32(open(p, "rb").read()), "08x")
img = "ble/ctl/out/ctl.img"
d = open(img, "rb").read()
magic, hv, hs, fw, size, hcrc = struct.unpack("<IHHIII", d[:20])
lines.append(f"ctl.img header crc32 {hcrc:08x}, image_size {size} B")
lines.append(f"ctl.img file crc32 {crc(img)}, {len(d)} B")
import os
def builds(pre):
    for p in sorted(["build/x0x-smk37.fwsc", "build/x0x-smk37-elite.fwsc", "build/x0x-smk37-elite-ble.fwsc",
                     "build/smk37/x0x.bin", "build/smk37/loader/ota.bin",
                     "build/smk37-elite/x0x.bin", "build/smk37-elite/loader/ota.bin"]):
        lines.append(f"{pre}{p} crc32 {crc(p)}, {len(open(p, 'rb').read())} B")
builds("")
os.chdir(sys.argv[2]); builds("rel/")
open(sys.argv[1], "w").write("\n".join(lines) + "\n")
PY
for pre in "" rel/; do
    grep -q "^${pre}build/x0x-smk37-elite-ble\.fwsc crc32 [0-9a-f]\{8\}, [0-9]* B\$" "$L/crc.txt" 2>/dev/null || fail "CRC manifest: no line for ${pre}build/x0x-smk37-elite-ble.fwsc"
done
mcrc() { sed -n "s|^$1 crc32 \([0-9a-f]\{8\}\),.*|\1|p" "$L/crc.txt" 2>/dev/null | head -1; }
CFG="dev elite $(mcrc build/x0x-smk37-elite.fwsc), ble $(mcrc build/x0x-smk37-elite-ble.fwsc), code free ${CODE_FREE:-?} B, pool ${POOL_H:-?} B; rel elite $(mcrc rel/build/x0x-smk37-elite.fwsc), ble $(mcrc rel/build/x0x-smk37-elite-ble.fwsc), code free ${CODE_FREE_R:-?} B, pool ${POOL_H_R:-?} B"
# Recheck the same effective inputs; never issue PASS if they changed during validation.
FINAL_INPUT_JSON=$(input_identity) || fail "cannot recheck validation inputs"
[ "$FINAL_INPUT_JSON" = "$INPUT_JSON" ] || fail "validation inputs changed during run"
T1=$(date +%s)
script_check
R=PASS; [ -n "$REASON" ] && R=FAIL
TMPF="$RF.tmp.$$"
{
echo "# Gate result for tree $TREE"
echo
echo "This file uses ASD Simplified Technical English."
echo
if [ "$TRIAL" = 1 ]; then echo "Result: TRIAL $R (a trial run with --trial: not a release PASS)"; else echo "Result: $R"; fi
[ -n "$REASON" ] && echo "Reason: $REASON"
echo
echo "- Purpose: $PURPOSE"
echo "- Run reason: $RUN_REASON"
[ "$FORCE" = 0 ] || echo "- Rerun reason: $FREASON"
echo "- Commit: $H"
echo "- Tree (SHA-1 of git ls-tree -r without *.md): $TREE"
echo "- Git tree (git rev-parse <commit>^{tree}): $GTREE"
echo "- References: pinned $PINS. Found:$PINLOG."
echo "- Private HOME and TMPDIR: $PH, $PT (rel: $PHR, $PTR)"
echo "- Configs: dev (X0X_DEV_PADCAL=1, worktree $W) and rel (X0X_DEV_PADCAL=0, the release build, worktree $WR). The two must pass."
echo "- Config CRCs: $CFG"
echo "- Validation inputs: $INPUT_ID"
echo "- Build variables of the caller (removed):${UNSET_TXT:- none}"
echo "- Time: $NOW. The gate took $((T1 - T0)) s."
echo "- Worktree: $W. Logs: $L."
echo
echo "## Builds"
echo
echo "- Elite build: $(grep -m1 '^package' "$L/build-elite.log" | sed "s|$W/||")"
echo "- Pro build: $(grep -m1 '^package' "$L/build-pro.log" | sed "s|$W/||")"
echo "- Bluetooth image: $(echo "$CTL_LINE" | sed "s|$W/||")"
echo
echo "## Sizes"
echo
echo "| Build | App code free | App RAM (.data+.bss) | Pool (the headroom limit is 8192 B) |"
echo "|---|---|---|---|"
printf '%s' "$SIZES"
echo
echo "| Item | Value |"
echo "|---|---|"
echo "| X0X_RAM_CANARY | $CANARY_TXT |"
echo "| ctl.img | ${CTL_SIZE:-?} of 65536 B, $(( 65536 - ${CTL_SIZE:-0} )) B free |"
echo "| Core RAM | ${CORE_USED:-?} of ${CORE_MAX:-?} B, $CORE_FREE B free |"
echo
echo "## Tests"
echo
echo "- Time scale (X0X_TEST_TIME_SCALE): $SCALE. The gate always uses 1: the real waits of the updater."
echo "- tests/run_tests.sh ($MODE): $OK ok, $NF FAIL, $NS skip. Exit code $TRC."
echo "- tests/run_tests.sh, rel config ($MODE): $OKR ok, $NFR FAIL, $NSR skip. Exit code $TRCR. Scenarios: $SCENR ok. Tests that did not run: ${MISSR:-none}."
echo "- Gate lock: $LOCK_TXT."
echo "- The script knows $NEXP tests. Tests that did not run: ${MISS:-none}."
echo "- Scenarios: $SCEN ok."
echo "- ble_test: ${BLE:-no log}"
echo "- smk_clip_keys_test: ${CK:-no log}"
echo
echo "## Flash map"
echo
echo '```'
cat "$L/flashmap.log"
echo '```'
echo
echo "## CRC manifest"
echo
echo '```'
cat "$L/crc.txt" 2>/dev/null
echo '```'
} > "$TMPF"
mv "$TMPF" "$RF"; chmod a+r "$RF"
CANON_TXT=""
if [ "$TRIAL" = 0 ] && [ "$R" = PASS ] && [ "$CANON_R" != PASS ]; then   # the first PASS of the tree is the canonical result
    cp "$RF" "$CANON.tmp.$$" && chmod a+r "$CANON.tmp.$$" && mv -f "$CANON.tmp.$$" "$CANON" && CANON_TXT=" (also $TREE.md)"
fi
echo "| $S | ${TREE:0:12} | ${GTREE:0:12} | $TP$R${FREASON:+ (force: $FREASON)} | $(basename "$RF")$CANON_TXT | $NOW | $CFG; purpose: $PURPOSE; reason: $RUN_REASON |" >> "$INDEX"
cp "$RF" "$L/result.md"
if [ "$R" = PASS ]; then echo "${TP}PASS $S (time scale $SCALE)"; exit 0; fi
echo "${TP}FAIL $S (time scale $SCALE) $REASON"; exit 1
