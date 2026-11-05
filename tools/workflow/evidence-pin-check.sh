#!/usr/bin/env bash
# Read-only: every 40-hex SHA and #832 comment id cited in a handoff/plan file must resolve.
# usage: evidence-pin-check.sh <file>   env REPOS="owner/a owner/b" ISSUE_REPO=owner/a
set -uo pipefail
f=$1
REPOS=${REPOS:-"gobbajogger/X0X37-for-Loopy-Pro gobbajogger/x0x37-for-Loopy-Pro_nightly"}
IR=${ISSUE_REPO:-gobbajogger/X0X37-for-Loopy-Pro}
H=(); [[ -n ${GITHUB_TOKEN:-} ]] && H=(-H "Authorization: Bearer $GITHUB_TOKEN")
rc=0
for s in $(grep -oE '\b[0-9a-f]{40}\b' "$f" | sort -u); do
  ok=""; for r in $REPOS; do
    for k in commits git/trees; do
      [[ $(curl -s --max-time 30 "${H[@]}" -o /dev/null -w '%{http_code}' "https://api.github.com/repos/$r/$k/$s") == 200 ]] && { ok="$r ${k#git/}"; break 2; }
    done; done
  [[ -n $ok ]] && echo "PASS ${s:0:8} ($ok)" || { echo "FAIL ${s:0:8} not found"; rc=1; }
done
for c in $(grep -oE '\b66[0-9]{8}\b' "$f" | sort -u); do
  [[ $(curl -s --max-time 30 "${H[@]}" -o /dev/null -w '%{http_code}' "https://api.github.com/repos/$IR/issues/comments/$c") == 200 ]] && echo "PASS comment $c" || { echo "FAIL comment $c"; rc=1; }
done
exit $rc
