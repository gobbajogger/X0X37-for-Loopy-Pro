#!/usr/bin/env bash
# Read-only: re-hash the live packages of a catalog entry against versions.json.
# usage: verify-hashes.sh [version-id]   (default: .latest)  env REF=gh-pages
set -euo pipefail
REF=${REF:-gh-pages}
url=https://raw.githubusercontent.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/$REF/versions.json
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
curl -fsS --max-time 30 "$url" -o "$tmp/v.json"
echo "versions.json sha256 $(sha256sum "$tmp/v.json" | cut -c1-64) entries $(jq '.versions|length' "$tmp/v.json")"
id=${1:-$(jq -r .latest "$tmp/v.json")}
jq -e --arg id "$id" '.versions[]|select(.version==$id)' "$tmp/v.json" >/dev/null || { echo "FAIL no entry $id"; exit 1; }
rc=0
while IFS=$'\t' read -r model pkg sha size; do
  curl -fsSL --max-time 30 "$pkg" -o "$tmp/p" || { echo "FAIL $model fetch $pkg"; rc=1; continue; }
  got=$(sha256sum "$tmp/p" | cut -c1-64); gs=$(stat -c%s "$tmp/p")
  if [[ $got == "$sha" && $gs == "$size" ]]; then echo "PASS $id $model ${sha:0:12} $size B"
  else echo "FAIL $id $model got ${got:0:12}/$gs want ${sha:0:12}/$size"; rc=1; fi
done < <(jq -r --arg id "$id" '.versions[]|select(.version==$id)|.models|to_entries[]|[.key,.value.pkg,.value.sha256,(.value.size|tostring)]|@tsv' "$tmp/v.json")
exit $rc
