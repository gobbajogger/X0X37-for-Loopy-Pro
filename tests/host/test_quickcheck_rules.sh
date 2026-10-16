#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# The rules step of tools/quickcheck.sh (checks 27 and 28b, rules = code), X0X_QUICK_ONLY=ste (no build, no tests),
# on a temp repository with integration, smk37 (docs/rules/, docs/USER_PREFERENCES.md) and lanes on smk37.
# Fixtures: tests/host/fixtures/rules_guard/bad-rule.txt (a bad rule text), tests/host/fixtures/prefs_lint/ (rules, good.md, bad.md).
#   rule change with "Trigger:" and "Check: <script of the tree>"      -> QUICK PASS, base = the smk37 merge base
#   no Trigger/Check lines, or a Check that names no script            -> QUICK FAIL ... RULES: ... commit body issues
#   "Check: manual: <reason>"                                           -> QUICK PASS
#   the bad rule text (no tools/ste_strict in the tree)                -> QUICK FAIL ... STE (rules mode is strict)
#   USER_PREFERENCES.md with an entry that names no rule                -> QUICK FAIL ... RULES: PREFS FAIL
#   a docs change that is not a rule change, good prefs                 -> QUICK PASS (docs lint runs)
#   the tip of lane/dk-rules-* with no rule file change, bad message   -> QUICK FAIL ... STE (rules mode by the branch name)
#   an integration lane commit                                          -> QUICK PASS, no RULES line, base = the integration merge base
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
unset X0X_STE_STRICT X0X_QC_BASE X0X_DOCS_BRANCH
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; Q="$TOP/tools/quickcheck.sh"; F="$TOP/tests/host/fixtures"
T=$(mktemp -d "${TMPDIR:-/tmp}/qcrules.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
R="$T/repo"; mkdir -p "$R" && cd "$R" && git init -q . && git config user.name t && git config user.email t@t && git checkout -q -b integration || { echo "FAIL setup"; exit 1; }
gc() { git commit -q "$@"; }
echo base > a.txt && git add . && gc -m "Add the first file." -m "The file has one line."
git checkout -q -b smk37 && mkdir -p docs/rules tools && cp "$F/prefs_lint/rules/"*.md docs/rules/ && cp "$F/prefs_lint/good.md" docs/USER_PREFERENCES.md &&
    printf '#!/bin/sh\necho check\n' > tools/check_x.sh && echo plan > docs/ROADMAP.md && git add . && gc -m "Add the docs." -m "The docs have the rules."
git checkout -q integration && echo more >> a.txt && git add . && gc -m "Change the first file." -m "The file has two lines."
lane() {  # lane <branch> <base> <file> <content file or text> <subject> <body>...
    b=$1; base=$2; f=$3; c=$4; s=$5; shift 5
    git checkout -q -b "$b" "$base"; if [ -f "$c" ]; then cp "$c" "$f"; else echo "$c" >> "$f"; fi
    git add . && args=(); for x in "$@"; do args+=(-m "$x"); done; gc -m "$s" "${args[@]}"; git rev-parse HEAD
}
GOOD=$(lane lane/dk-rules-good smk37 docs/rules/install.md "Run each install in the foreground." "Add a rule fact." "The rule has one more fact." $'Trigger: P9 install.\nCheck: tools/check_x.sh')
NOTRIG=$(lane lane/dk-rules-notrig smk37 docs/rules/install.md "Run each install in the foreground." "Add a rule fact." "The rule has one more fact.")
BADCHK=$(lane lane/dk-rules-badchk smk37 docs/rules/install.md "Run each install in the foreground." "Add a rule fact." "The rule has one more fact." $'Trigger: P9 install.\nCheck: tools/nothere.sh')
MANUAL=$(lane lane/dk-rules-manual smk37 docs/rules/install.md "Run each install in the foreground." "Add a rule fact." "The rule has one more fact." $'Trigger: P9 install.\nCheck: manual: no script can see the screen session.')
BADTXT=$(lane lane/dk-rules-badtxt smk37 docs/rules/install.md "$F/rules_guard/bad-rule.txt" "Change rule 7." "The rule has new text." $'Trigger: P9 install.\nCheck: tools/check_x.sh')
BADPREF=$(lane lane/dk-rules-badpref smk37 docs/USER_PREFERENCES.md "$F/prefs_lint/bad.md" "Add the owner entries." "The file has six new entries." $'Trigger: each owner decision.\nCheck: tools/check_x.sh')
AGR=$(lane lane/dk-rules-agr smk37 docs/agent-rules.md "Read the core rules first." "Add the agent rules." "The file points to the core rules.")
DOCS=$(lane docs-roadmap smk37 docs/ROADMAP.md "Batch 9 is next." "Update the plan." "The plan names the next batch.")
TIPMSG=$(lane lane/dk-rules-tip smk37 docs/ROADMAP.md "Batch 9 is next." "Fix the plan; it was changed" "The plan was changed by the tool.")
INTL=$(lane lane/btr-x integration a.txt "three" "Change the file." "The file has three lines.")
git checkout -q integration; cd - >/dev/null
touch "$T/pre-push"
qc() { X0X_QUICK_ONLY=ste X0X_REPO="$R" X0X_QC_WORK="$T/qc" X0X_PREPUSH_HOOK="$T/pre-push" bash "$Q" "$@" 2>&1; }
chk() {  # chk <name> <PASS|FAIL> <grep -E pattern or ""> <commit> [<qc.log pattern>]
    name=$1; want=$2; pat=$3; c=$4; lp=${5:-}; n=$((n + 1))
    out=$(qc "$c"); rc=$?
    q=$(echo "$out" | grep '^QUICK ' | tail -1)
    if echo "$q" | grep -q "^QUICK $want " && { [ -z "$pat" ] || echo "$out" | grep -qE -- "$pat"; } &&
       { [ -z "$lp" ] || grep -qE -- "$lp" "$T/qc/wt/build/qc/qc.log"; }; then echo "ok   $name: $q"
    else echo "FAIL $name (want $want, rc=$rc)"; echo "$out" | sed 's/^/     /'; [ -n "$lp" ] && head -3 "$T/qc/wt/build/qc/qc.log" | sed 's/^/     log: /'; fails=$((fails + 1)); fi
}
chk "rule change, Trigger and Check" PASS "" "$GOOD" "changed: docs/rules/install.md$"
chk "rule change, no Trigger or Check" FAIL "RULES: 2 commit body issues" "$NOTRIG"
chk "rule change, Check names no script" FAIL "nothere.sh is not a script" "$BADCHK"
chk "rule change, Check: manual" PASS "" "$MANUAL"
chk "bad rule text, rules mode is strict" FAIL "QUICK FAIL .* STE: " "$BADTXT"
chk "USER_PREFERENCES.md entry with no rule" FAIL "RULES: PREFS FAIL 6 of 6 entries" "$BADPREF"
chk "docs/agent-rules.md change, no Trigger or Check" FAIL "RULES: 2 commit body issues" "$AGR"
chk "docs change, not a rule change" PASS "" "$DOCS" "changed: docs/ROADMAP.md$"
chk "lane/dk-rules tip, bad message" FAIL "QUICK FAIL .* STE: " "$TIPMSG" "rules mode \(refs/remotes/origin/lane/dk-rules-tip\)"
n=$((n + 1)); out=$(qc "$INTL"); base=$(git -C "$R" merge-base "$INTL" integration | cut -c1-7)
if echo "$out" | grep -q '^QUICK PASS ' && ! echo "$out" | grep -q '^RULES' && grep -q "base $base, changed: a.txt$" "$T/qc/wt/build/qc/qc.log"; then
    echo "ok   integration lane: $(echo "$out" | grep '^QUICK'), base $base"
else echo "FAIL integration lane"; echo "$out" | sed 's/^/     /'; head -2 "$T/qc/wt/build/qc/qc.log"; fails=$((fails + 1)); fi
echo "quickcheck rules: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
