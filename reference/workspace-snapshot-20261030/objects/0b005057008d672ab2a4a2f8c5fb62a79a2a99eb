#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Quick check before a push or a handoff (Linux build environment, no device). Target: 60 s or less. It is not the gate.
#   tools/quickcheck.sh <commit or ref>     a commit of the box repository (its own worktree, kept for incremental builds)
#   tools/quickcheck.sh <worktree folder>   a worktree as it is (with changes that are not committed)
# Steps:
#   1. plan scope; build Elite only for target/unknown/mixed or empty scope. Scope errors fail closed.
#   2. Run affected tests. Proven documentation changes skip the target build; unknown inputs retain host coverage.
# The changed files: the commit against X0X_QC_BASE. The default base is <commit>^ for a commit on integration,
# and the merge base with integration for other commits. For a folder, the changes that are not committed are added.
# Output: one line, "QUICK PASS <hash> <seconds> s" or "QUICK FAIL <hash> <seconds> s <reason>". Exit code 0 or 1.
# The log is in <work>/qc.log. Environment (defaults in brackets):
#   X0X_REPO [/workspace/x0x37.git]  X0X_QC_WORK [~/.cache/x0x37-quickcheck]  X0X_QC_BASE [see above]
#   X0X_INTEGRATION [integration]  JIELI_TOOLCHAIN [~/.jieli/toolchain]  AC79_SDK [~/fw-AC79_AIoT_SDK]
#   X0X_REFS [/workspace/x0x37-refs/schwung]  X0X_PREPUSH_HOOK [/workspace/x0x37.git/hooks/pre-push]
# The quick check is a dev run: it never takes the gate lock (/workspace/x0x37-refs/gate.lock). Only the gate does.
# Device bug step, after the tests:
#   A commit in BASE..<commit> whose message has the words "device bug" must add a file tests/*inlog* (the replay test of
#   the INLOG log, or its log file), unless the message has the line "INLOG skip: owner <HH:MM>" (the owner skips the
#   INLOG step for this bug). Bug evidence: <changed tests/path> permits a source-proven regression instead.
#   The regression must pass on the fixed tree. The replay test = each run_tests.sh test (of that commit) whose command names an added
#   tests/*inlog* file, or names a file that names it. That test must FAIL on the parent of the commit: the step runs it in
#   a temp worktree of the parent, with the tests/ files of the commit. A test that passes there, an inlog file that no
#   test uses, or no inlog file makes "QUICK FAIL ... DEVICE BUG: ...". X0X_QUICK_ONLY=devbug: only this step.
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE   # a hook sets these; a scratch repository must not use the repository of the hook
set -uo pipefail
T0=$(date +%s)
QCDIR=$(cd "$(dirname "$0")" && pwd)
REPO=${X0X_REPO:-$(git -C "$QCDIR/.." rev-parse --show-toplevel 2>/dev/null || echo /workspace/x0x37.git)}
QW=${X0X_QC_WORK:-$HOME/.cache/x0x37-quickcheck}
INT=${X0X_INTEGRATION:-integration}
DOCB=${X0X_DOCS_BRANCH:-smk37}
TC=${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}
SDK=${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}
REFS=${X0X_REFS:-/workspace/x0x37-refs/schwung}
PREPUSH=${X0X_PREPUSH_HOOK:-$QCDIR/pre-push.hook}
[ -n "${1:-}" ] || { echo "usage: tools/quickcheck.sh <commit or worktree folder>" >&2; exit 2; }
secs() { echo "$(( $(date +%s) - T0 ))"; }
H=${1}
out() { echo "QUICK $1 ${H:0:7} $(secs) s${2:+ $2}"; [ "$1" = PASS ]; exit $?; }
ancestor() {
    "${G[@]}" merge-base --is-ancestor "$1" "$2" 2>/dev/null; local rc=$?
    [ "$rc" -le 1 ] || out FAIL "scope: cannot read ancestry"
    return "$rc"
}
if [ ! -d "$1" ] && ! command -v flock >/dev/null 2>&1; then
    out FAIL "setup: commit-mode quickcheck needs flock; use the Linux container or a worktree folder for docs-only checks"
fi
if [ -d "$1" ]; then                                    # a worktree folder, as it is
    W=$(cd "$1" && pwd); G=(git -C "$W")
    H=$("${G[@]}" rev-parse HEAD) || { H=$1; out FAIL "not a git worktree"; }
    IREF=$("${G[@]}" for-each-ref --count=1 --format='%(refname)' "refs/remotes/*/$INT") || out FAIL "scope: cannot read integration refs"
    DREF=$("${G[@]}" for-each-ref --count=1 --format='%(refname)' "refs/remotes/*/$DOCB") || out FAIL "scope: cannot read docs refs"
else                                                    # a commit: its own worktree in $QW/wt
    mkdir -p "$QW" || out FAIL "setup: cannot create $QW"
    exec 9>"$QW/.lock" || out FAIL "setup: cannot open $QW/.lock"
    flock -w "${X0X_QC_LOCK_WAIT:-120}" 9 || out FAIL "setup: quickcheck work folder is busy ($QW/.lock); retry after its owner finishes"
    [ -d "$QW/.repo" ] || git clone -q --no-local --no-checkout "$REPO" "$QW/.repo" || out FAIL "git clone of $REPO failed"
    HC=$(git -C "$QW/.repo" rev-parse --path-format=absolute --git-common-dir)
    cp "$PREPUSH" "$HC/hooks/pre-push" && chmod +x "$HC/hooks/pre-push" || out FAIL "cannot copy the pre-push hook $PREPUSH"
    for rem in $(git -C "$QW/.repo" remote); do
        [ "$(git -C "$QW/.repo" remote get-url --push "$rem")" = /workspace/x0x37.git ] ||
            git -C "$QW/.repo" remote set-url --push "$rem" DISABLED-no-push-outside-box
    done
    FE=$(git -C "$QW/.repo" fetch -q origin '+refs/heads/*:refs/remotes/origin/*' 2>&1) || out FAIL "git fetch: $(echo "$FE" | tail -n1)"
    H=$(git -C "$QW/.repo" rev-parse --verify --quiet "$1^{commit}" || git -C "$QW/.repo" rev-parse --verify --quiet "origin/$1^{commit}") ||
        { H=$1; out FAIL "unknown commit"; }
    W="$QW/wt"
    if [ -d "$W" ]; then git -C "$W" checkout -q -f --detach "$H" || out FAIL "checkout failed"
    else git -C "$QW/.repo" worktree prune; git -C "$QW/.repo" worktree add -q --detach "$W" "$H" || out FAIL "worktree failed"; fi
    G=(git -C "$W"); IREF="refs/remotes/origin/$INT"
    DREF=$("${G[@]}" for-each-ref --count=1 --format='%(refname)' "refs/remotes/origin/$DOCB") || out FAIL "scope: cannot read docs refs"
fi
L="$W/build/qc"; mkdir -p "$L" "$L/tmp"; LOG="$L/qc.log"; : > "$LOG"
if [ -n "${X0X_QC_BASE:-}" ]; then BASE=$("${G[@]}" rev-parse --verify "$X0X_QC_BASE^{commit}") || out FAIL "scope: invalid base"
elif [ -n "$IREF" ] && ancestor "$H" "$IREF"; then BASE=$("${G[@]}" rev-parse "$H^") || out FAIL "scope: no parent"
elif [ -n "$DREF" ] && ancestor "$H" "$DREF"; then BASE=$("${G[@]}" rev-parse "$H^") || out FAIL "scope: no parent"
elif [ -n "$IREF" ]; then BASE=$("${G[@]}" merge-base "$H" "$IREF") || out FAIL "scope: no integration merge base"
    if [ -n "$DREF" ]; then
        MD=$("${G[@]}" merge-base "$H" "$DREF") || out FAIL "scope: no docs merge base"
        if [ "$MD" != "$BASE" ] && ancestor "$BASE" "$MD"; then BASE=$MD; fi
    fi
else BASE=$("${G[@]}" rev-parse "$H^") || out FAIL "scope: no parent"; fi
PLAN_ARGS=(); [ -d "$1" ] && PLAN_ARGS+=(--folder)
case "${X0X_QUICK_ONLY:-}" in devbug) PLAN_ARGS+=(--scope-only) ;; esac
python3 "$QCDIR/quickcheck_plan.py" "$W" "$BASE" "$H" "${PLAN_ARGS[@]}" > "$L/plan.json" 2> "$L/plan-error.log" || {
    cat "$L/plan-error.log" >> "$LOG"; out FAIL "scope: cannot read complete plan (see $L/plan-error.log)";
}
# JSON preserves Git names. Shell fields below contain only booleans or runner test names.
python3 - "$L/plan.json" > "$L/plan-fields" <<'PYPLAN'
import json, sys
p = json.load(open(sys.argv[1]))
for k in ('target', 'docs', 'checks'): print(int(p[k]))
print(' '.join(p['tests']))
PYPLAN
{ read -r TARGET; read -r DOCS_CH; read -r NEED_TESTS; read -r SEL; } < "$L/plan-fields"
CHANGED=$(python3 -c 'import json,sys; print("\n".join(json.load(open(sys.argv[1]))["changed"]))' "$L/plan.json")
echo "quickcheck ${H:0:7}, base ${BASE:0:7}, changed: $(echo $CHANGED)" >> "$LOG"
cat "$L/plan.json" >> "$LOG"
cd "$W" || out FAIL "no worktree"
export JIELI_TOOLCHAIN="$TC" AC79_SDK="$SDK" TMPDIR="$L/tmp"
export NINEW9_DIR="${NINEW9_DIR:-$REFS/schwung-9W9}" EIGHTW8_DIR="${EIGHTW8_DIR:-$REFS/schwung-8W8}" SCHWUNG303="${SCHWUNG303:-$REFS/schwung-303}"
export TB3PO_REF="${TB3PO_REF:-$REFS/schwung-tb3po/src/dsp/tb3po.c}" X0X_TEST_JOBS=8 X0X_TEST_TIME_SCALE=20

# Device bug step (see the header). It sets DB_FAIL (1 = a failure) and DB_NOTE.
devbug_step() {
    DB_FAIL=0; DB_NOTE=""
    local c msg added names wt r errs=""
    for c in $("${G[@]}" rev-list --no-merges "$BASE..$H" 2>/dev/null); do
        msg=$("${G[@]}" log -1 --format=%B "$c")
        echo "$msg" | grep -qi 'device bug' || continue
        added=$("${G[@]}" diff-tree --no-commit-id -r --diff-filter=A --name-only "$c" | grep -E '^tests/.*inlog' || true)
        # A source-proven regression can replace new device logging.
        evidence=$(printf '%s\n' "$msg" | sed -n 's/^Bug evidence: *//p')
        if [ -n "$evidence" ]; then
            added=$("${G[@]}" diff-tree --no-commit-id -r --diff-filter=AM --name-only "$c" -- tests | grep -F -x -- "$evidence" || true)
            [ -n "$added" ] || { errs="$errs; ${c:0:7}: Bug evidence must name an added or changed tests/ file"; continue; }
        fi
        if [ -z "$added" ]; then
            echo "$msg" | grep -qE '^INLOG skip: owner [0-9]{1,2}:[0-9]{2}' && { echo "devbug ${c:0:7}: owner INLOG skip" >> "$LOG"; continue; }
            errs="$errs; ${c:0:7} adds no tests/*inlog* replay test"; continue
        fi
        names=$("${G[@]}" show "$c:tests/run_tests.sh" 2>/dev/null | QC_ADDED="$added" QC_C="$c" python3 -c '
import os, re, subprocess, sys
src = sys.stdin.read().replace("\\\n", " "); added = os.environ["QC_ADDED"].split(); g = sys.argv[1:]; c = os.environ["QC_C"]
def show(p):
    r = subprocess.run(g + ["show", c + ":" + p], capture_output=True, text=True, errors="ignore")
    return r.stdout if r.returncode == 0 else ""
out = set()
for l in src.split("\n"):
    if l.lstrip().startswith("#"): continue
    for m in re.finditer(r"(?:^|[\s;&|])run \"?([A-Za-z0-9_.-]+)\"?\s+(.*)", l):
        n, cmd = m.group(1), m.group(2)
        files = re.findall(r"tests/[\w./-]+", cmd)
        if any(a in cmd for a in added) or any(os.path.basename(a) in show(f) for f in files for a in added): out.add(n)
print(" ".join(sorted(out)))' "${G[@]}")
        [ -n "$names" ] || { errs="$errs; ${c:0:7}: no run_tests.sh test uses $(echo $added)"; continue; }
        wt=$(mktemp -d "${TMPDIR:-/tmp}/qcdevbug.XXXXXX")
        (cd "$W" && X0X_TESTS="$names" sh tests/run_tests.sh > "$wt/fixed.log" 2>&1); fixed_rc=$?
        cat "$wt/fixed.log" >> "$LOG"
        for n in $names; do
            [ "$fixed_rc" = 0 ] && grep -qE "^ *ok +$n( |\$)" "$wt/fixed.log" || errs="$errs; ${c:0:7}: replay test $n does not pass on the fixed tree"
        done
        if "${G[@]}" worktree add -q --detach "$wt/w" "$c^" >/dev/null 2>&1; then
            "${G[@]}" diff-tree --no-commit-id -r --diff-filter=AM --name-only "$c" -- tests | while read -r f; do
                mkdir -p "$wt/w/$(dirname "$f")"; "${G[@]}" show "$c:$f" > "$wt/w/$f"; done
            (cd "$wt/w" && X0X_TESTS="$names" sh tests/run_tests.sh > "$wt/base.log" 2>&1); r=$?
            cat "$wt/base.log" >> "$LOG"
            for n in $names; do
                grep -qE "^ *FAIL +$n( |\$)" "$wt/base.log" || errs="$errs; ${c:0:7}: the replay test $n does not fail on the parent ${c:0:7}^ (rc $r)"
            done
            "${G[@]}" worktree remove -f "$wt/w" >/dev/null 2>&1
        else errs="$errs; ${c:0:7}: no worktree of the parent"; fi
        rm -rf "$wt"
        echo "devbug ${c:0:7}: inlog $(echo $added), tests $names" >> "$LOG"
    done
    [ -z "$errs" ] || { DB_FAIL=1; DB_NOTE="DEVICE BUG: ${errs#; }"; }
    return 0
}
if [ "${X0X_QUICK_ONLY:-}" = devbug ]; then
    devbug_step; [ "$DB_FAIL" = 1 ] && out FAIL "$DB_NOTE"; out PASS "(device bug step only)"
fi
# Planning completes before any target process starts.
if [ "$TARGET" = 1 ]; then
    if [ "$(uname -s)" != Linux ]; then
        out FAIL "build validation needs Linux tools; use the local Linux container (BUILDING.md)."
    fi
    if ! MODEL=elite ./build.sh --board smk37 > "$L/build-elite.log" 2>&1 9>&-; then
        out FAIL "Elite build: $(grep -m1 -E 'FAIL|error|Error' "$L/build-elite.log" | sed 's/^ *//' | cut -c1-160)"
    fi
else
    echo "target build skipped: proven non-target scope" >> "$LOG"
fi

echo "selected: $SEL" >> "$LOG"
TRC=0; NF=0; NOK=0; MISS=""; : > "$L/tests.log"
if [ -n "$SEL" ]; then
    RT=tests/run_tests.sh
    [ -f "$RT" ] || out FAIL "tests: runner missing"
    if ! grep -q 'X0X_TESTS' "$RT"; then
        RT=tests/.qc_run_tests.sh
        sed 's/^run() {$/run() { case " $X0X_QC_SEL " in *" $1 "*) ;; *) return ;; esac/' tests/run_tests.sh > "$RT"
        export X0X_QC_SEL="$SEL"
    fi
    X0X_TESTS="$SEL" sh "$RT" > "$L/tests.log" 2>&1 9>&-; TRC=$?
    [ "$RT" = tests/run_tests.sh ] || rm -f "$RT"
    cat "$L/tests.log" >> "$LOG"
    NF=$(grep -c '^  FAIL' "$L/tests.log"); NOK=$(grep -c '^  ok ' "$L/tests.log")
    for n in $SEL; do grep -qE "^  ok +$n( |\$)" "$L/tests.log" || MISS="$MISS $n"; done
elif [ "$NEED_TESTS" = 1 ]; then
    out FAIL "tests: no runnable tests selected for relevant scope"
fi
devbug_step; [ "$DB_FAIL" = 1 ] && out FAIL "$DB_NOTE"
[ "$NF" = 0 ] && [ "$TRC" = 0 ] && [ -z "$MISS" ] || out FAIL "tests: $NF failed; missing PASS:$MISS (runner rc $TRC)"
out PASS "($NOK tests ok)"
