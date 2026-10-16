#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/ste_check.py --owner-msg (check 2, rule 13 and core rule 6A): a message to the owner has 1 to 3 sentences,
# 20 words or fewer in each, no hex of 7 or more characters, no path or file name, no backticks, at most one "?",
# and no question about an install. Fixtures: tests/host/fixtures/owner_msg/good-*.txt (exit 0, "OWNER MSG OK")
# and bad-*.txt (exit 1, "OWNER MSG FAIL" with the named reason). Also stdin ("-").
set -u
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; S="$TOP/tools/ste_check.py"; F="$TOP/tests/host/fixtures/owner_msg"
fails=0; n=0
chk() {  # chk <file> <want rc> <want text>
    n=$((n + 1)); out=$(python3 "$S" --owner-msg "$F/$1" 2>&1); rc=$?
    if [ "$rc" = "$2" ] && printf '%s\n' "$out" | grep -qF -- "$3"; then echo "ok   $1: $(printf '%s' "$out" | head -1)"
    else echo "FAIL $1: rc=$rc (want $2), want \"$3\""; printf '%s\n' "$out" | sed 's/^/     /'; fails=$((fails + 1)); fi
}
chk good-result.txt 0 "OWNER MSG OK"
chk good-question.txt 0 "OWNER MSG OK"
chk bad-four-sentences.txt 1 "4 sentences (max 3)"
chk bad-long-sentence.txt 1 "words (max 20)"
chk bad-hex.txt 1 "hex 7f32160"
chk bad-path.txt 1 "path tools/fm1_install.py"
chk bad-backticks.txt 1 "backticks"
chk bad-two-questions.txt 1 "2 question marks (max 1)"
chk bad-install-question.txt 1 "a question about an install"
chk bad-empty.txt 1 "no sentence"
n=$((n + 1)); out=$(printf 'The pads work again.\n' | python3 "$S" --owner-msg - 2>&1); rc=$?
if [ "$rc" = 0 ] && echo "$out" | grep -q "OWNER MSG OK"; then echo "ok   stdin"; else echo "FAIL stdin: rc=$rc $out"; fails=$((fails + 1)); fi
n=$((n + 1)); out=$(python3 "$S" "$F/good-result.txt" 2>&1); rc=$?
if echo "$out" | grep -q '^file'; then echo "ok   normal mode unchanged"; else echo "FAIL normal mode: $out"; fails=$((fails + 1)); fi
echo "ste owner msg: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
