#!/usr/bin/env bash
# Usage: make_package.sh <sha> <image_dir> <testlog>
# Copies images + test log into /workspace/release/<shortsha>/ and fills CRC32/SHA-256. No install, no push.
set -euo pipefail
[ $# -eq 3 ] || { echo "usage: $0 <sha> <image_dir> <testlog>" >&2; exit 2; }
SHA=$1; IMG=$2; LOG=$3
[[ $SHA =~ ^[0-9a-f]{40}$ ]] || { echo "sha must be 40 hex chars" >&2; exit 2; }
[ -d "$IMG" ] && [ -f "$LOG" ] || { echo "image_dir or testlog missing" >&2; exit 2; }
T=$(cd "$(dirname "$0")" && pwd); S=${SHA:0:7}; OUT=${RELEASE_ROOT:-/workspace/release}/$S
[ -e "$OUT" ] && { echo "$OUT exists; not overwriting" >&2; exit 1; }
mkdir -p "$OUT/images" "$OUT/logs"
cp -r "$T/hw-checklists" "$T/source-snapshots" "$OUT/"
cp "$LOG" "$OUT/logs/host-tests-$S.log"
TABLE=""; : > "$OUT/SHA256SUMS"; : > "$OUT/CRC32SUMS"
shopt -s nullglob; n=0
for f in "$IMG"/*; do [ -f "$f" ] || continue; b=$(basename "$f"); cp "$f" "$OUT/images/$b"
  crc=$(python3 -c 'import sys,zlib;print("%08x"%(zlib.crc32(open(sys.argv[1],"rb").read())&0xffffffff))' "$OUT/images/$b")
  sha=$(sha256sum "$OUT/images/$b" | cut -d' ' -f1); sz=$(stat -c %s "$OUT/images/$b")
  TABLE+="| images/$b | $sz | $crc | $sha |"$'\n'; echo "$sha  images/$b" >> "$OUT/SHA256SUMS"; echo "$crc  images/$b" >> "$OUT/CRC32SUMS"; n=$((n+1)); done
[ $n -gt 0 ] || { echo "no image files in $IMG" >&2; rm -rf "$OUT"; exit 1; }
SRC=$(realpath "$LOG") TABLE="${TABLE%$'\n'}" SHA=$SHA S=$S python3 - "$T/README.md" "$OUT/README.md" <<'PY'
import os,sys
t=open(sys.argv[1]).read()
for k,v in {"{{IMAGE_TABLE}}":os.environ["TABLE"],"{{SHORTSHA}}":os.environ["S"],"{{SHA}}":os.environ["SHA"],"{{TESTLOG_SRC}}":os.environ["SRC"]}.items(): t=t.replace(k,v)
t=t.replace("Status: TEMPLATE — NOT FILLED.","Status: DRAFT — checksums filled; other {{ }} fields still open.")
open(sys.argv[2],"w").write(t)
PY
echo "package: $OUT ($n images)"
