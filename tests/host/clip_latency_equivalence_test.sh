#!/bin/sh
# Compile the actual foreground clip functions against deterministic host inputs.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
python3 - "$root/firmware/src/app/main_fm1.c" "$tmp/candidate.inc" <<'PY'
import pathlib
import sys

source = pathlib.Path(sys.argv[1]).read_text()
blocks = []
for signature in ("CK_FN static uint32_t ck_key_notes(",
                  "CK_FN static void ck_arp_step("):
    start = source.index(signature)
    body = source.index("{", start)
    depth = 1
    end = body + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    blocks.append(source[start:end])
pathlib.Path(sys.argv[2]).write_text(
    blocks[0] + "\n#define ck_key_notes counted_key_notes\n" + blocks[1])
PY
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$tmp" -I"$root/firmware/src" \
    "$root/tests/host/clip_latency_equivalence_test.c" -o "$tmp/test"
"$tmp/test"
