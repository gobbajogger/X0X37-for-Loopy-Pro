#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Quick check before a push or a handoff (box only, no device). Target: 60 s or less. It is not the gate.
#   tools/quickcheck.sh <commit or ref>     a commit of the box repository (its own worktree, kept for incremental builds)
#   tools/quickcheck.sh <worktree folder>   a worktree as it is (with changes that are not committed)
# Steps:
#   1. the Elite build (MODEL=elite ./build.sh --board smk37), incremental. It includes the build checks.
#   2. only the tests that the changed files affect (see select_tests below), parallel (X0X_TEST_JOBS=8), time scale 20.
#      If no test matches a changed file, a fast host-only set runs.
# The changed files: the commit against X0X_QC_BASE. The default base is <commit>^ for a commit on integration,
# and the merge base with integration for other commits. For a folder, the changes that are not committed are added.
# Output: one line, "QUICK PASS <hash> <seconds> s" or "QUICK FAIL <hash> <seconds> s <reason>". Exit code 0 or 1.
# The log is in <work>/qc.log. Environment (defaults in brackets):
#   X0X_REPO [/workspace/x0x37.git]  X0X_QC_WORK [~/.cache/x0x37-quickcheck]  X0X_QC_BASE [see above]
#   X0X_INTEGRATION [integration]  JIELI_TOOLCHAIN [~/.jieli/toolchain]  AC79_SDK [~/fw-AC79_AIoT_SDK]
#   X0X_REFS [/workspace/x0x37-refs/schwung]  X0X_PREPUSH_HOOK [/workspace/x0x37.git/hooks/pre-push]
# The quick check is a dev run: it never takes the gate lock (/workspace/x0x37-refs/gate.lock). Only the gate does.
# STE step (tools/ste_check.py, the copy beside this script): it examines the changed *.md files and the
# message (subject and body) of each commit in BASE..<commit>. Counted categories: long_proc, long_desc,
# semicolon, modal, passive, contraction, no_verb. The default is warn only: "STE WARN <n> issues" and the
# first lines come before the QUICK line, and the exit code does not change. Strict mode: the file
# tools/ste_strict in the tree, or X0X_STE_STRICT=1. In strict mode, an issue makes "QUICK FAIL ... STE: ...".
# X0X_QUICK_ONLY=ste: only the STE step and the rules step (no build, no tests).
# Rules step (checks 27 and 28b of rules v4, owner 02:37: rules = code):
#   Docs lint: a commit that changes docs/ or USER_PREFERENCES.md, in a tree with docs/USER_PREFERENCES.md and docs/rules/,
#   runs tools/prefs_lint.py (the copy beside this script). A "PREFS FAIL" makes "QUICK FAIL ... RULES: ...".
#   Rules mode: the changes touch docs/rules/, docs/agent-rules.md, docs/USER_PREFERENCES.md or USER_PREFERENCES.md, or <commit> is the tip of
#   a lane/dk-rules-* branch. In rules mode the STE step is strict, the docs lint runs, and each commit in BASE..<commit>
#   that changes these paths must have two body lines:
#     "Trigger: <when the rule applies>"   (2 words or more, the "Read when" or "Step" of the rule)
#     "Check: <script>"                     a path in the tree (tools/quickcheck.sh), a file name in tools/ of the tree
#                                           (require_gate_pass.sh), status.sh or x0x37_gate.sh (the live scripts),
#                                           or "Check: manual: <reason>" when no script can check the rule.
#   More "Check:" lines are allowed. Each one must be valid. A missing line or a bad value makes "QUICK FAIL ... RULES: ...".
#   A missing prefs_lint.py in rules mode or with a docs change is a failure (fail closed).
# Base for a commit on a docs lane: the merge base with X0X_DOCS_BRANCH [smk37] when it is newer than the merge base
# with integration (a commit on X0X_DOCS_BRANCH itself: <commit>^).
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE   # a hook sets these; a scratch repository must not use the repository of the hook
set -u
T0=$(date +%s%N)
QCDIR=$(cd "$(dirname "$0")" && pwd)
REPO=${X0X_REPO:-/workspace/x0x37.git}
QW=${X0X_QC_WORK:-$HOME/.cache/x0x37-quickcheck}
INT=${X0X_INTEGRATION:-integration}
DOCB=${X0X_DOCS_BRANCH:-smk37}
TC=${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}
SDK=${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}
REFS=${X0X_REFS:-/workspace/x0x37-refs/schwung}
PREPUSH=${X0X_PREPUSH_HOOK:-/workspace/x0x37.git/hooks/pre-push}
[ -n "${1:-}" ] || { echo "usage: tools/quickcheck.sh <commit or worktree folder>" >&2; exit 2; }
secs() { local t=$(( ($(date +%s%N) - T0) / 1000000 )); echo "$((t / 1000)).$(( (t % 1000) / 100 ))"; }
H=${1}
out() { echo "QUICK $1 ${H:0:7} $(secs) s${2:+ $2}"; [ "$1" = PASS ]; exit $?; }
mkdir -p "$QW"
if [ -d "$1" ]; then                                    # a worktree folder, as it is
    W=$(cd "$1" && pwd); G=(git -C "$W")
    H=$("${G[@]}" rev-parse HEAD) || { H=$1; out FAIL "not a git worktree"; }
    IREF=$("${G[@]}" for-each-ref --format='%(refname)' "refs/remotes/*/$INT" | head -1)
    DREF=$("${G[@]}" for-each-ref --format='%(refname)' "refs/remotes/*/$DOCB" | head -1)
    RLANE=$("${G[@]}" for-each-ref --points-at HEAD --format='%(refname)' 'refs/heads/lane/dk-rules-*' 'refs/remotes/*/lane/dk-rules-*' | head -1)
else                                                    # a commit: its own worktree in $QW/wt
    exec 9>"$QW/.lock"; flock -n 9 || { echo "quickcheck: waiting for $QW/.lock (another quickcheck)" >&2; flock 9; }
    [ -d "$QW/.repo" ] || git clone -q --no-checkout "$REPO" "$QW/.repo" || out FAIL "git clone of $REPO failed"
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
    DREF=$("${G[@]}" rev-parse -q --verify "refs/remotes/origin/$DOCB" >/dev/null && echo "refs/remotes/origin/$DOCB")
    RLANE=$("${G[@]}" for-each-ref --points-at "$H" --format='%(refname)' 'refs/remotes/origin/lane/dk-rules-*' | head -1)
fi
L="$W/build/qc"; mkdir -p "$L" "$L/home" "$L/tmp"; LOG="$L/qc.log"; : > "$LOG"
if [ -n "${X0X_QC_BASE:-}" ]; then BASE=$("${G[@]}" rev-parse "$X0X_QC_BASE")
elif [ -n "$IREF" ] && "${G[@]}" merge-base --is-ancestor "$H" "$IREF" 2>/dev/null; then BASE=$("${G[@]}" rev-parse "$H^")
elif [ -n "$DREF" ] && "${G[@]}" merge-base --is-ancestor "$H" "$DREF" 2>/dev/null; then BASE=$("${G[@]}" rev-parse "$H^")
elif [ -n "$IREF" ]; then BASE=$("${G[@]}" merge-base "$H" "$IREF")
    if [ -n "$DREF" ] && MD=$("${G[@]}" merge-base "$H" "$DREF" 2>/dev/null) && [ "$MD" != "$BASE" ] &&
       "${G[@]}" merge-base --is-ancestor "$BASE" "$MD" 2>/dev/null; then BASE=$MD; fi   # a docs lane: the newer base
else BASE=$("${G[@]}" rev-parse "$H^"); fi
CHANGED=$( { "${G[@]}" diff --name-only "$BASE" "$H"; [ -d "$1" ] && "${G[@]}" diff --name-only HEAD; } | sort -u)
echo "quickcheck ${H:0:7}, base ${BASE:0:7}, changed: $(echo $CHANGED)" >> "$LOG"
RULES_MODE=0; DOCS_CH=0
for f in $CHANGED; do
    case "$f" in docs/rules/*|docs/agent-rules.md|docs/USER_PREFERENCES.md|USER_PREFERENCES.md) RULES_MODE=1; DOCS_CH=1 ;; docs/*) DOCS_CH=1 ;; esac
done
[ -n "${RLANE:-}" ] && RULES_MODE=1
[ "$RULES_MODE" = 1 ] && echo "rules mode (${RLANE:-rule files changed})" >> "$LOG"
cd "$W" || out FAIL "no worktree"
export JIELI_TOOLCHAIN="$TC" AC79_SDK="$SDK" HOME="$L/home" TMPDIR="$L/tmp"
export NINEW9_DIR="$REFS/schwung-9W9" EIGHTW8_DIR="$REFS/schwung-8W8" SCHWUNG303="$REFS/schwung-303"
export TB3PO_REF="$REFS/schwung-tb3po/src/dsp/tb3po.c" X0X_TEST_JOBS=8 X0X_TEST_TIME_SCALE=20

# STE step (see the header). It sets STE_FAIL (1 = strict and issues) and STE_NOTE.
ste_step() {
    STE_FAIL=0; STE_NOTE=""
    local d n strict=0 st="$QCDIR/ste_check.py" c f md=""
    { [ -f "$W/tools/ste_strict" ] || [ "${X0X_STE_STRICT:-0}" = 1 ] || [ "$RULES_MODE" = 1 ]; } && strict=1
    if [ ! -f "$st" ]; then                             # strict mode fails closed: no check is a failure
        echo "STE: no $st, STE step skipped" >> "$LOG"
        [ $strict = 1 ] && { STE_FAIL=1; STE_NOTE="; STE WARN 1 issues: no $st (run tools/install_gate.sh)"; }
        return 0
    fi
    if ! grep -q -- '"--diff"' "$st"; then                # the changed-lines mode (ste_check.py --diff): else fail closed
        echo "STE: $st has no --diff mode" >> "$LOG"
        [ $strict = 1 ] && { STE_FAIL=1; STE_NOTE="; STE WARN 1 issues: $st has no --diff mode (run tools/install_gate.sh)"; }
        return 0
    fi
    d=$(mktemp -d "${TMPDIR:-/tmp}/qcste.XXXXXX") || return 0
    for c in $("${G[@]}" rev-list --no-merges "$BASE..$H" 2>/dev/null); do
        { echo "${c:0:7}"; "${G[@]}" log -1 --format=%B "$c"; echo "@@END@@"; } > "$d/commit-${c:0:7}"
    done
    for f in $CHANGED; do case "$f" in *.md) [ -f "$W/$f" ] && md="$md $W/$f" ;; esac; done
    n=$(ls "$d" | wc -l)
    if [ "$n" -gt 0 ] || [ -n "$md" ]; then
        STE_OUT=$(cd "$d" && python3 "$st" --json --examples 50 ${md:+--diff "$W" "$BASE" WORKTREE} $(ls "$d") $md 2>&1 | QC_W="$W/" python3 -c '
import json, os, sys
K = ["long_proc", "long_desc", "semicolon", "modal", "passive", "contraction", "no_verb"]
try: rs = json.load(sys.stdin)
except Exception as e: print("0 ste_check.py output not read: %s" % e); sys.exit()
n = sum(r["counts"][k] for r in rs for k in K); lines = []
for r in rs:
    name = r["file"].replace(os.environ["QC_W"], "").replace("commit-", "commit ")
    for e in r["worst"]:
        ks = [k for k in e["rules"] if k in K]
        if ks: lines.append("STE  %s: [%s] %s" % (name, ",".join(ks), e["text"][:140]))
print(n); print("\n".join(lines[:5]))')
        n=$(echo "$STE_OUT" | head -1)
        echo "STE ($([ $strict = 1 ] && echo strict || echo warn)): $n issues" >> "$LOG"; echo "$STE_OUT" | tail -n +2 >> "$LOG"
        if [ "${n:-0}" -gt 0 ] 2>/dev/null; then
            echo "STE WARN $n issues"; echo "$STE_OUT" | tail -n +2
            STE_NOTE="; STE WARN $n issues"
            [ $strict = 1 ] && STE_FAIL=1
        fi
    fi
    rm -rf "$d"
}
# Rules step (see the header). It sets RULES_FAIL (1 = a failure) and RULES_NOTE.
rules_step() {
    RULES_FAIL=0; RULES_NOTE=""
    local pl="$QCDIR/prefs_lint.py" o msgs=""
    if [ "$DOCS_CH" = 1 ] && [ -f "$W/docs/USER_PREFERENCES.md" ] && [ -d "$W/docs/rules" ]; then
        if [ ! -f "$pl" ]; then msgs="no $pl (run tools/install_gate.sh)"
        else
            o=$(python3 "$pl" --rules "$W/docs/rules" "$W/docs/USER_PREFERENCES.md" 2>&1)
            echo "$o" >> "$LOG"
            case "$o" in "PREFS OK"*) ;; *) msgs="$(echo "$o" | head -1)"; echo "RULES $o" | head -6 ;; esac
        fi
    elif [ "$DOCS_CH" = 1 ]; then echo "rules: no docs/USER_PREFERENCES.md or docs/rules/ in the tree, no docs lint" >> "$LOG"
    fi
    if [ "$RULES_MODE" = 1 ]; then
        o=$(for c in $("${G[@]}" rev-list --no-merges "$BASE..$H" -- docs/rules docs/agent-rules.md USER_PREFERENCES.md docs/USER_PREFERENCES.md 2>/dev/null); do
            QC_C=$c QC_TOOLS="$("${G[@]}" ls-tree --name-only "$H:tools" 2>/dev/null)" python3 -c '
import os, re, subprocess, sys
c = os.environ["QC_C"]; g = sys.argv[1:]
body = subprocess.run(g + ["log", "-1", "--format=%b", c], capture_output=True, text=True).stdout
tools = set(os.environ["QC_TOOLS"].split()) | {"status.sh", "x0x37_gate.sh"}
bad = []
if not re.search(r"^Trigger: *\S+ +\S+", body, re.M): bad.append("no line Trigger: <when the rule applies>")
checks = re.findall(r"^Check: *(.*)$", body, re.M)
if not checks: bad.append("no line Check: <script>")
for v in checks:
    v = v.strip(); w = v.split()[0].rstrip(".,") if v.split() else ""
    if re.match(r"manual: *\S+", v): continue
    ok = w in tools or subprocess.run(g + ["cat-file", "-e", "HEAD:" + w], capture_output=True).returncode == 0 if w else False
    if not ok: bad.append("Check: %s is not a script of the tree, status.sh, x0x37_gate.sh or manual: <reason>" % v[:60])
for b in bad: print("commit %s: %s" % (c[:7], b))
' "${G[@]}"; done)
        if [ -n "$o" ]; then echo "$o" >> "$LOG"; echo "$o" | head -5 | sed 's/^/RULES  /'; msgs="${msgs:+$msgs, }$(echo "$o" | wc -l | tr -d ' ') commit body issues"; fi
    fi
    if [ -n "$msgs" ]; then RULES_FAIL=1; RULES_NOTE="RULES: $msgs"; fi
    return 0
}
if [ "${X0X_QUICK_ONLY:-}" = ste ]; then
    ste_step; [ "$STE_FAIL" = 1 ] && out FAIL "STE: ${STE_NOTE#; STE WARN } (strict)"
    rules_step; [ "$RULES_FAIL" = 1 ] && out FAIL "$RULES_NOTE"
    out PASS "(STE step only${STE_NOTE})"
fi

# 1. the Elite build (incremental), with its build checks
if ! MODEL=elite ./build.sh --board smk37 > "$L/build-elite.log" 2>&1 9>&-; then
    out FAIL "Elite build: $(grep -m1 -E 'FAIL|error|Error' "$L/build-elite.log" | sed 's/^ *//' | cut -c1-160)"
fi

# 2. the tests that the changed files affect
SEL=$(QC_CHANGED="$CHANGED" python3 - <<'PY'
import glob, os, re, sys
changed = [l.strip() for l in os.environ["QC_CHANGED"].split("\n") if l.strip()]
src = open("tests/run_tests.sh").read().replace("\\\n", " ")
cmds = {}                                        # test name -> command text
for l in src.split("\n"):
    if l.lstrip().startswith("#"): continue
    for m in re.finditer(r'(?:^|[\s;&|])run "?([A-Za-z0-9_.-]+)"?\s+(.*)', l):
        cmds.setdefault(m.group(1), m.group(2))
for t in ("drum909", "drum808", "bass303", "breaks"):
    if os.path.exists(f"tests/host/run_{t}.sh"): cmds[t] = f"sh tests/host/run_{t}.sh"
scen = ["scenario-" + os.path.basename(s)[:-4] for s in sorted(glob.glob("tests/scenarios/*.x0x"))]
FAST = "fastmath sequencer encoder uac trs undo lz motion storage smk-flash smk-storage smk-apc40-map smk-clip-keys locked-settings".split()
RULES = [                                        # path prefix -> tests (added to the tests found by text)
    ("firmware/src/dsp/drum909", "drum909 drum909-adpcm drum-pan"), ("firmware/src/dsp/drum808", "drum808 drum-pan"),
    ("firmware/src/dsp/bass303", "bass303 bass303_lite"), ("firmware/src/dsp/breaks", "breaks"),
    ("firmware/src/dsp/", "master stereo fastmath"), ("firmware/src/seq/", "sequencer motion loop-bars rec-live"),
    ("firmware/src/storage", "storage smk-storage"), ("firmware/hal/fm1_flash_map.h", "smk-flash storage smk-storage"),
    ("firmware/src/ble", "ble smk-clip-ble-clear smk-clip-ble-notify diag-size"), ("firmware/src/lcd", "smk-clip-dark"),
    ("tools/fm1_install.py", "installer installer-slots installer-ctl installer-targets"), ("tools/fm1pkg_make.py", "smk-package"),
    ("tools/upload_breaks.py", "upload-images upload-tools breaks-web"), ("web/", "breaks-web breaks-page install-slots-web"),
    ("tools/locked_settings.json", "locked-settings"),
]
sel = set(["locked-settings", "smk-flash"])
for p in changed:
    if p.endswith(".md") or p.startswith("docs/"): continue
    hit = set(n for n, c in cmds.items() if p in c)                      # the test command names the file
    base = os.path.basename(p)
    for f in glob.glob("tests/**/*", recursive=True):                    # a test source that names the file
        if os.path.isfile(f) and f != p and not f.startswith("tests/scenarios/"):
            try:
                if base in open(f, errors="ignore").read(): hit |= set(n for n, c in cmds.items() if f in c)
            except OSError: pass
    for pre, t in RULES:
        if p.startswith(pre): hit |= set(t.split())
    if p.startswith("tests/scenarios/"): hit |= {"host-build", "scenario-" + base[:-4]}
    if p.startswith(("firmware/src/app/", "firmware/src/seq/", "firmware/src/dsp/", "host/")):
        hit |= {"host-build"} | set(scen)                                # the whole app in the simulator
    if p.startswith("firmware/") and not hit: hit |= set(FAST)
    if p == "tests/run_tests.sh": hit |= set(FAST)
    sel |= hit
if any(n.startswith("scenario-") for n in sel): sel |= {"host-build", "upload-images"}   # the scenarios need both
if len(sel) <= 2: sel |= set(FAST)                                      # nothing matched: the fast host-only set
print(" ".join(sorted(n for n in sel if n in cmds or n.startswith("scenario-"))))
PY
)
echo "selected: $SEL" >> "$LOG"
RT=tests/run_tests.sh
if ! grep -q 'X0X_TESTS' "$RT"; then                    # an older tree: a copy of run_tests.sh with the name filter
    RT=tests/.qc_run_tests.sh
    sed 's/^run() {$/run() { case " $X0X_QC_SEL " in *" $1 "*) ;; *) return ;; esac/' tests/run_tests.sh > "$RT"
    export X0X_QC_SEL="$SEL"
fi
X0X_TESTS="$SEL" sh "$RT" > "$L/tests.log" 2>&1 9>&-; TRC=$?
[ "$RT" = tests/run_tests.sh ] || rm -f "$RT"
cat "$L/tests.log" >> "$LOG"
NF=$(grep -c '^  FAIL' "$L/tests.log"); NOK=$(grep -c '^  ok ' "$L/tests.log")
MISS=""; for n in $SEL; do grep -qE "^  (ok|FAIL) +$n( |\$)" "$L/tests.log" || MISS="$MISS $n"; done
WT=""; ste_step; WT="$STE_NOTE"; [ "$STE_FAIL" = 1 ] && [ "$NF" = 0 ] && [ "$TRC" = 0 ] && out FAIL "STE: ${STE_NOTE#; STE WARN } (strict)"
rules_step; [ "$RULES_FAIL" = 1 ] && [ "$NF" = 0 ] && [ "$TRC" = 0 ] && out FAIL "$RULES_NOTE"
[ "$NF" = 0 ] && [ "$TRC" = 0 ] || out FAIL "tests: $NF failed ($(grep '^  FAIL' "$L/tests.log" | awk '{print $2}' | tr '\n' ' ' | sed 's/ $//'))$WT"
out PASS "($NOK tests ok${MISS:+; not run:$MISS}$WT)"
