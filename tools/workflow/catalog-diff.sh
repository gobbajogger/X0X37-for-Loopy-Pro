#!/usr/bin/env bash
# Read-only: diff versions.json between two nightly refs (commit SHAs or branches).
# Flags REMOVED existing entries and status/label/hardware_tested/source_commit/models
# changes on an EXISTING entry (needs owner's word). Exit 2 is advisory for the push
# gate; a human/publisher reads the output.
# usage: catalog-diff.sh <old-ref> <new-ref>
set -euo pipefail
raw=https://raw.githubusercontent.com/gobbajogger/x0x37-for-Loopy-Pro_nightly
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
curl -fsS --max-time 30 "$raw/$1/versions.json" -o "$tmp/a"; curl -fsS --max-time 30 "$raw/$2/versions.json" -o "$tmp/b"
ids(){ jq -r '.versions[].version' "$1" | sort; }
echo "latest: $(jq -r .latest "$tmp/a") -> $(jq -r .latest "$tmp/b")"
comm -13 <(ids "$tmp/a") <(ids "$tmp/b") | sed 's/^/ADDED   /'
rc=0
while IFS= read -r id; do
  [[ -n $id ]] || continue
  echo "REMOVED $id"
  rc=2
done < <(comm -23 <(ids "$tmp/a") <(ids "$tmp/b"))
for id in $(comm -12 <(ids "$tmp/a") <(ids "$tmp/b")); do
  for f in status label hardware_tested source_commit models; do
    x=$(jq -c --arg i "$id" --arg f "$f" '.versions[]|select(.version==$i)|.[$f]' "$tmp/a")
    y=$(jq -c --arg i "$id" --arg f "$f" '.versions[]|select(.version==$i)|.[$f]' "$tmp/b")
    if [[ $x != "$y" ]]; then
      echo "OWNER-WORD-NEEDED $id $f: $x -> $y"
      rc=2
    fi
  done
done
[[ $rc == 0 ]] && echo "PASS no status change on existing entries"
exit $rc
