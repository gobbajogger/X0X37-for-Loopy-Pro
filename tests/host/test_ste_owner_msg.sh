#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Owner messages allow necessary evidence, concise fragments and required questions.
# Old prohibition fixtures now pass; an empty message still fails.
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
chk bad-four-sentences.txt 0 "OWNER MSG OK"
chk bad-long-sentence.txt 0 "OWNER MSG OK"
chk bad-hex.txt 0 "OWNER MSG OK"
chk bad-path.txt 0 "OWNER MSG OK"
chk bad-backticks.txt 0 "OWNER MSG OK"
chk bad-two-questions.txt 0 "OWNER MSG OK"
chk bad-install-question.txt 0 "OWNER MSG OK"
chk bad-empty.txt 1 "empty message"
n=$((n + 1)); out=$(printf 'The pads work again.\n' | python3 "$S" --owner-msg - 2>&1); rc=$?
if [ "$rc" = 0 ] && echo "$out" | grep -q "OWNER MSG OK"; then echo "ok   stdin"; else echo "FAIL stdin: rc=$rc $out"; fails=$((fails + 1)); fi
n=$((n + 1)); out=$(python3 "$S" "$F/good-result.txt" 2>&1); rc=$?
if echo "$out" | grep -q '^file'; then echo "ok   normal mode unchanged"; else echo "FAIL normal mode: $out"; fails=$((fails + 1)); fi
echo "ste owner msg: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
