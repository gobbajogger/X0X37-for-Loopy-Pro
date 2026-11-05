#!/usr/bin/env bash
# Read-only: confirm a frozen candidate is still what was authorized.
# usage: verify-freeze.sh <owner/repo> <branch> <sha40> [tree40]
set -euo pipefail
repo=$1 br=$2 sha=$3 tree=${4:-}
api=https://api.github.com/repos/$repo
head=$(curl -fsS --max-time 30 "$api/branches/$br" | jq -r .commit.sha)
c=$(curl -fsS --max-time 30 "$api/git/commits/$sha")
t=$(jq -r .tree.sha <<<"$c"); a=$(jq -r '.author.email' <<<"$c"); m=$(jq -r .message <<<"$c")
rc=0
[[ $head == "$sha" ]] && echo "PASS head $br = ${sha:0:8}" || { echo "FAIL head $br = ${head:0:8}, expected ${sha:0:8}"; rc=1; }
if [[ -n $tree ]]; then [[ $t == "$tree" ]] && echo "PASS tree ${t:0:8}" || { echo "FAIL tree ${t:0:8}, expected ${tree:0:8}"; rc=1; }; fi
[[ $a == *users.noreply.github.com ]] && echo "PASS author $a" || { echo "FAIL author $a"; rc=1; }
grep -qiE '^(co-authored-by|signed-off-by):' <<<"$m" && { echo "FAIL trailer in message"; rc=1; } || echo "PASS no trailers"
exit $rc
