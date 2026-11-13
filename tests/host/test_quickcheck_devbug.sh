#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# The device bug step of tools/quickcheck.sh (checks 16 and 17, rule 8), X0X_QUICK_ONLY=devbug, on a temp repository.
# Fixtures: tests/host/fixtures/devbug/ (run_tests.sh, pad_inlog_test.sh: a good replay test, weak_inlog_test.sh: a test
# that passes on the parent).
#   "device bug" commit + good tests/*inlog* test         -> QUICK PASS
#   "device bug" commit, no inlog file                     -> QUICK FAIL ... DEVICE BUG: ... adds no tests/*inlog*
#   "device bug" commit, "INLOG skip: owner 01:49"         -> QUICK PASS
#   "device bug" commit + a test that passes on the parent -> QUICK FAIL ... does not fail on the parent
#   "device bug" commit + an inlog file that no test uses  -> QUICK FAIL ... no run_tests.sh test uses
#   a log file tests/fixtures/*inlog* + a test that reads it (good) -> QUICK PASS
#   a commit without "device bug"                          -> QUICK PASS
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
unset X0X_QC_BASE X0X_TESTS
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; Q="$TOP/tools/quickcheck.sh"; F="$TOP/tests/host/fixtures/devbug"
T=$(mktemp -d "${TMPDIR:-/tmp}/qcdevbug.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
R="$T/repo"; mkdir -p "$R/tests" && cd "$R" && git init -q . && git config user.name t && git config user.email t@t && git checkout -q -b integration || { echo "FAIL setup"; exit 1; }
echo bug > app.txt && cp "$F/run_tests.sh" tests/ && git add . && git commit -qm "Add the app." -m "The app has one line."
mk() {  # mk <branch> <subject> <body> <files: src:dst ...>: the commit hash
    b=$1; s=$2; body=$3; shift 3; git checkout -q -b "$b" integration; echo fixed > app.txt
    for p in "$@"; do mkdir -p "$(dirname "${p#*:}")"; cp "${p%%:*}" "${p#*:}"; done
    git add . && git commit -qm "$s" -m "$body" && git rev-parse HEAD
}
GOOD=$(mk good "Clip pads: fix the dark pad." "This fixes a device bug. The replay test uses the INLOG log." "$F/pad_inlog_test.sh:tests/pad_inlog_test.sh")
NOLOG=$(mk nolog "Clip pads: fix the dark pad." "This fixes a device bug.")
SKIP=$(mk skip "Clip pads: fix the dark pad." $'This fixes a device bug.\nINLOG skip: owner 01:49')
WEAK=$(mk weak "Clip pads: fix the dark pad." "This fixes a device bug." "$F/weak_inlog_test.sh:tests/weak_inlog_test.sh")
printf 'pad 5 dark\n' > "$T/log.txt"; printf '#!/bin/sh\ngrep -q dark tests/fixtures/pad_inlog.txt && grep -qx fixed app.txt\n' > "$T/replay.sh"
UNUSED=$(mk unused "Clip pads: fix the dark pad." "This fixes a device bug." "$T/log.txt:tests/fixtures/x_inlog.txt")
VIAFILE=$(mk viafile "Clip pads: fix the dark pad." "This fixes a device bug." "$T/log.txt:tests/fixtures/pad_inlog.txt" "$T/replay.sh:tests/replay_test.sh")
SOURCE=$(mk source "Fix the source-proven device bug." $'Bug evidence: tests/replay_test.sh' "$T/replay.sh:tests/replay_test.sh" "$T/log.txt:tests/fixtures/pad_inlog.txt")
BADSOURCE=$(mk badsource "Fix the device bug." $'Bug evidence: app.txt')
PLAIN=$(mk plain "Change the app." "The app has the new line.")
git checkout -q integration; cd - >/dev/null; touch "$T/pre-push"
chk() {  # chk <name> <PASS|FAIL> <grep -E pattern or ""> <commit>
    n=$((n + 1)); out=$(X0X_QUICK_ONLY=devbug X0X_REPO="$R" X0X_QC_WORK="$T/qc" X0X_PREPUSH_HOOK="$T/pre-push" bash "$Q" "$4" 2>&1); rc=$?
    q=$(echo "$out" | grep '^QUICK ' | tail -1)
    if echo "$q" | grep -q "^QUICK $2 " && { [ -z "$3" ] || echo "$q" | grep -qE -- "$3"; }; then echo "ok   $1: $q"
    else echo "FAIL $1 (want $2, rc=$rc)"; echo "$out" | sed 's/^/     /'; fails=$((fails + 1)); fi
}
chk "device bug, good replay test" PASS "" "$GOOD"
chk "device bug, no inlog test" FAIL "adds no tests/\*inlog\* replay test" "$NOLOG"
chk "device bug, owner INLOG skip" PASS "" "$SKIP"
chk "device bug, test passes on the parent" FAIL "replay test weak-inlog does not fail on the parent" "$WEAK"
chk "device bug, inlog file that no test uses" FAIL "no run_tests.sh test uses tests/fixtures/x_inlog.txt" "$UNUSED"
chk "device bug, inlog log read by a test" PASS "" "$VIAFILE"
chk "source-proven bug with regression" PASS "" "$SOURCE"
chk "source evidence is not a test" FAIL "Bug evidence must name" "$BADSOURCE"
chk "no device bug" PASS "" "$PLAIN"
echo "quickcheck device bug: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
