#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Pre-install gate for the SMK-37 Elite firmware (Linux x86-64 box only). It does not touch a device.
#   tools/gate.sh <commit or ref> [--force "<reason>"] [--trial]
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
#   1. the Elite build, the Pro build (build/x0x-smk37.fwsc, for the package tests), the FM-1 build and
#      the Bluetooth image build (private /tmp),
#   2. tests/run_tests.sh with AC79_SDK and the reference variables (parallel if the tree has X0X_TEST_JOBS).
#      The gate compares the tests that ran with the list of tests in the script. A missing test is a failure.
#   3. the flash-map check (tools/gate_flashmap.py, with the owner tag), the budget checks for the three builds,
#      the X0X_RAM_CANARY default (it must be 0) and a CRC manifest.
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
GATE_DIR=$(cd "$(dirname "$0")" && pwd)
REPO=${X0X_REPO:-/workspace/x0x37.git}
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
PREPUSH=${X0X_PREPUSH_HOOK:-/workspace/x0x37.git/hooks/pre-push}
GLOCK=${X0X_GATE_LOCK:-/workspace/x0x37-refs/gate.lock}
PINS=${X0X_REF_PINS:-schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d}
INTB=${X0X_INTEGRATION:-integration}
FORCE=0; TRIAL=0; ARG=""; FREASON=""
while [ $# -gt 0 ]; do
    case "$1" in
    --force) FORCE=1; FREASON=${2:-}; [ $# -ge 2 ] && shift
             case "$FREASON" in ""|-*|[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]) FREASON="" ;; esac
             [ -n "$FREASON" ] || { echo "REFUSED: --force needs a reason: tools/gate.sh <commit> --force \"<reason>\"" >&2; exit 2; } ;;
    --trial) TRIAL=1 ;;
    -*) echo "unknown option $1" >&2; exit 2 ;;
    *) ARG=$1 ;;
    esac
    shift
done
set -- "$ARG"
[ -n "${1:-}" ] || { echo "usage: tools/gate.sh <commit or ref> [--force \"<reason>\"] [--trial]" >&2; exit 2; }
FREASON=$(printf '%s' "$FREASON" | tr '|\n' '/ ' | cut -c1-120)

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
    [ -n "${LOG:-}" ] && [ -d "$(dirname "$LOG")" ] && echo "gate: KILLED by $1 (no result)" >> "$LOG"
    echo "KILLED $S $1 (no result)" >&7
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
NOW=$(date '+%Y-%m-%d %H:%M %Z')
mkdir -p "$RESULTS" && chmod a+rx "$RESULTS"
INDEX="$RESULTS/index.md"
INDEX_NOTE='Each line gives the commit, the tree, the git tree, the result and the time. The key is the tree: the SHA-1 of "git ls-tree -r --full-tree" without the *.md files. The git tree is "git rev-parse <commit>^{tree}". Older rows (before the Git tree column) have no git tree. In some older rows, the Tree column has the git ^{tree} and the text "(git ^{tree})".'
[ -f "$INDEX" ] || printf '# x0x37 gate results: index\n\n%s\n\n| Commit | Tree | Git tree | Result | Result file | Time |\n|---|---|---|---|---|---|\n' "$INDEX_NOTE" > "$INDEX"
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
chmod a+r "$INDEX"
CANON="$RESULTS/$TREE.md"                # the first PASS of the tree; the gate never replaces a PASS here
CANON_R=""; [ -f "$CANON" ] && CANON_R=$(sed -n 's/^Result: \([A-Z]*\).*/\1/p' "$CANON" | head -1)
STAMP=$(date '+%Y%m%d-%H%M')
RF="$RESULTS/$TREE-$STAMP.md"; [ "$TRIAL" = 1 ] && RF="$RESULTS/$TREE.trial-$STAMP.md"
[ -e "$RF" ] && RF="${RF%.md}-$$.md"      # two runs in one minute
if [ "$FORCE" = 0 ] && [ "$TRIAL" = 0 ] && [ "$CANON_R" = PASS ]; then
    echo "| $S | ${TREE:0:12} | ${GTREE:0:12} | PASS (result used again) | $TREE.md | $NOW |" >> "$INDEX"
    OSC=$(sed -n 's/^- Time scale (X0X_TEST_TIME_SCALE): \([0-9]*\).*/\1/p' "$CANON" | head -1)
    echo "PASS $S (time scale ${OSC:-1}) (result used again: tree ${TREE:0:12} has a PASS)"; exit 0
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
[ -d "$WORK/.repo" ] || git clone -q --no-checkout "$REPO" "$WORK/.repo" || { echo "FAIL $S git clone of $REPO failed"; exit 1; }
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
if [ -d "$W" ]; then
    git -C "$W" checkout -q --detach "$H" && git -C "$W" reset -q --hard "$H"
else
    git -C "$WORK/.repo" worktree prune
    git -C "$WORK/.repo" worktree add -q --detach "$W" "$H"
fi
L="$WORK/$S.logs"; rm -rf "$L"; mkdir -p "$L"
LOG="$L/gate.log"
T0=$(date +%s)
REASON=""
fail() { [ -n "$REASON" ] || REASON="$1"; echo "gate: FAIL $1" >> "$LOG"; }

cd "$W" || exit 1
PH="$L/home"; PT="$L/tmp"; mkdir -p "$PH" "$PT"
export HOME="$PH" TMPDIR="$PT"          # a private HOME and TMPDIR for all builds and tests of this run
mkdir -p "$PH/.jieli"; ln -sfn "$TC" "$PH/.jieli/toolchain"; ln -sfn "$SDK" "$PH/fw-AC79_AIoT_SDK"   # the real toolchain and SDK
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
MODEL=elite child ./build.sh --board smk37 > "$L/build-elite.log" 2>&1 9>&- || fail "Elite build: $(grep -m1 -iE 'error|fail' "$L/build-elite.log")"
child ./build.sh --board smk37 > "$L/build-pro.log" 2>&1 9>&- || fail "Pro build: $(grep -m1 -iE 'error|fail' "$L/build-pro.log")"
child ./build.sh > "$L/build-fm1.log" 2>&1 9>&- || fail "FM-1 build: $(grep -m1 -iE 'error|fail' "$L/build-fm1.log")"
PCT="$L/ctl-tmp"; mkdir -p "$PCT"
child unshare -rm sh -c "mount --bind '$PCT' /tmp && env HOME='$PH' JIELI_TOOLCHAIN='$TC' SDK_LOCAL='$SDK' CTL_NATIVE=1 sh ble/ctl/build_ctl.sh" \
    > "$L/build-ctl.log" 2>&1 || fail "ctl build: $(grep -m1 -iE 'error|fail|undefined' "$L/build-ctl.log")"

# 2. tests
MODE="X0X_TEST_JOBS=$JOBS"
grep -q X0X_TEST_JOBS tests/run_tests.sh || MODE="serial: this tree has no parallel mode"
# The box-wide gate lock: one gate test phase at a time on this box. The lock file is writable by all users.
[ -e "$GLOCK" ] || { ( umask 000; : >> "$GLOCK" ) && chmod a+rw "$GLOCK"; } 2>/dev/null
LW0=$(date +%s)
if ! exec 9>>"$GLOCK"; then
    fail "cannot open the gate lock $GLOCK"; TRC=1; LOCK_TXT="not taken (cannot open $GLOCK)"
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
    X0X_TEST_JOBS="$JOBS" child sh tests/run_tests.sh > "$L/tests.log" 2>&1 9>&-; TRC=$?
    flock -u 9; exec 9>&-
    LW2=$(date +%s)
    echo "gate: released the gate lock; the tests took $((LW2 - LW1)) s" >> "$LOG"
    LOCK_TXT="$GLOCK: waited $((LW1 - LW0)) s, held $((LW2 - LW1)) s"
fi
OK=$(grep -c '^  ok ' "$L/tests.log"); NF=$(grep -c '^  FAIL' "$L/tests.log"); NS=$(grep -c '^  skip' "$L/tests.log")
[ "$TRC" = 0 ] || fail "tests: $NF failed ($(grep -m1 '^  FAIL' "$L/tests.log" | sed 's/^  FAIL //'))"
[ "$NS" = 0 ] || fail "tests: $NS skipped"
BLE=$(tail -n1 build/host/ble.log 2>/dev/null)
CK=$(tail -n1 build/host/smk-clip-keys.log 2>/dev/null)
SCEN=$(grep -c '^  ok   scenario-' "$L/tests.log")
# The tests that the script knows. A test that does not run (and does not write a "skip" line) is a failure.
python3 - "$L/tests.log" "$L/tests-expected.txt" <<'PY' > "$L/tests-missing.txt"
import glob, os, re, shutil, sys
src = [l for l in open("tests/run_tests.sh").read().split("\n") if not l.lstrip().startswith("#")]
names = set()
for l in src:
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
if not shutil.which("emcc"): names -= {"emu", "emu-slots"}                                  # the browser build needs emcc
ran = set(re.findall(r"^  (?:ok|FAIL|skip) +(\S+)", open(sys.argv[1]).read(), re.M))
open(sys.argv[2], "w").write("\n".join(sorted(names)) + "\n")
print(" ".join(sorted(names - ran)))
PY
NEXP=$(grep -c . "$L/tests-expected.txt")
MISS=$(cat "$L/tests-missing.txt")
[ -z "$MISS" ] || fail "tests did not run: $MISS"
for t in smk-build-package ble-package; do grep -q "^  ok   $t\$" "$L/tests.log" || fail "test $t did not pass"; done

# 3. flash map, budgets, CRCs
python3 "$GATE_DIR/gate_flashmap.py" firmware/hal/fm1_flash_map.h firmware/src/storage.c > "$L/flashmap.log" 2>&1 || fail "$(tail -n1 "$L/flashmap.log")"
FM=$(tail -n1 "$L/flashmap.log")
num() { grep -m1 -oE "$1" "$2" | grep -oE '[0-9]+' | sed -n "${3}p"; }
SIZES=""
for b in elite pro fm1; do      # code free, RAM free and pool headroom for each build: all must be in the limits
    f="$L/build-$b.log"
    cf=$(num 'app code area [0-9]+ B, [0-9]+ B free' "$f" 2)
    ru=$(num 'RAM \.data\+\.bss [0-9]+ B of [0-9]+' "$f" 1); rm=$(num 'RAM \.data\+\.bss [0-9]+ B of [0-9]+' "$f" 2)
    pu=$(num 'pool [0-9]+ B of [0-9]+' "$f" 1); pm=$(num 'pool [0-9]+ B of [0-9]+' "$f" 2)
    rf=$(( ${rm:-0} - ${ru:-0} )); ph=$(( ${pm:-0} - ${pu:-0} ))
    [ -n "$cf" ] && [ "$cf" -ge 0 ] || fail "$b build: app code over the limit (no 'B free' line)"
    [ -n "$ru" ] && [ -n "$rm" ] && [ "$rf" -ge 0 ] || fail "$b build: app RAM over the limit"
    [ -n "$pu" ] && [ "$ph" -ge 8192 ] || fail "$b build: pool headroom $ph B < 8192 B"
    SIZES="$SIZES| $b | ${cf:-?} B | ${ru:-?} of ${rm:-?} B, $rf B free | ${pu:-?} of ${pm:-?} B, $ph B headroom |
"
    [ "$b" = elite ] && { CODE_FREE=$cf; RAM_USED=$ru; RAM_MAX=$rm; POOL=$pu; POOL_MAX=$pm; }
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
python3 - "$L/crc.txt" <<'PY' || fail "CRC manifest"
import glob, struct, sys, zlib
lines = []
def crc(p): return format(zlib.crc32(open(p, "rb").read()), "08x")
img = "ble/ctl/out/ctl.img"
d = open(img, "rb").read()
magic, hv, hs, fw, size, hcrc = struct.unpack("<IHHIII", d[:20])
lines.append(f"ctl.img header crc32 {hcrc:08x}, image_size {size} B")
lines.append(f"ctl.img file crc32 {crc(img)}, {len(d)} B")
for p in sorted(glob.glob("build/*.fwsc") + glob.glob("build/x0x.bin") + glob.glob("build/*/x0x.bin")
                + glob.glob("build/loader/ota.bin") + glob.glob("build/*/loader/ota.bin")):
    lines.append(f"{p} crc32 {crc(p)}, {len(open(p, 'rb').read())} B")
open(sys.argv[1], "w").write("\n".join(lines) + "\n")
PY
T1=$(date +%s)
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
echo "- Commit: $H"
echo "- Tree (SHA-1 of git ls-tree -r without *.md): $TREE"
echo "- Git tree (git rev-parse <commit>^{tree}): $GTREE"
echo "- References: pinned $PINS. Found:$PINLOG."
echo "- Private HOME and TMPDIR: $PH, $PT"
echo "- Time: $NOW. The gate took $((T1 - T0)) s."
echo "- Worktree: $W. Logs: $L."
echo
echo "## Builds"
echo
echo "- Elite build: $(grep -m1 '^package' "$L/build-elite.log" | sed "s|$W/||")"
echo "- Pro build: $(grep -m1 '^package' "$L/build-pro.log" | sed "s|$W/||")"
echo "- FM-1 build: $(grep -m1 '^package' "$L/build-fm1.log" | sed "s|$W/||")"
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
echo "| $S | ${TREE:0:12} | ${GTREE:0:12} | $TP$R${FREASON:+ (force: $FREASON)} | $(basename "$RF")$CANON_TXT | $NOW |" >> "$INDEX"
cp "$RF" "$L/result.md"
if [ "$R" = PASS ]; then echo "${TP}PASS $S (time scale $SCALE)"; exit 0; fi
echo "${TP}FAIL $S (time scale $SCALE) $REASON"; exit 1
