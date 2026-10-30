#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# One-way copy of the gate results from the box to the owner's Mac, for tools/require_gate_pass.sh on the Mac.
#   Box: tools/sync_results_to_mac.sh --pack          -> prints the path of the new tar file
#   Mac: tools/sync_results_to_mac.sh --unpack <tar>  -> updates the results folder on the Mac
# Transport: the same as the firmware files. The Mac cannot connect to the box. The agent copies the
# tar file from the box to the Mac with CopyFromBox (as for the .fwsc, ctl.img and .bundle files in
# /workspace/a65-artifacts), then runs --unpack on the Mac.
# The tar file has (paths below "gate-results/"):
#   index.md, QUEUE.md (if it is there), the result files that index.md names (<tree>[-<stamp>].md, <tree>.trial-<stamp>.md),
#   logs/<logs path without the first "/">/{gate.log,crc.txt,result.md,validation-inputs.json} for each "Logs:" path of a
#   result file (only these evidence files, not the worktrees), SHA256SUMS.
# Environment (defaults in brackets):
#   --pack:   X0X_RESULTS_DIR [/workspace/x0x37-results]   X0X_SYNC_DIR [$X0X_RESULTS_DIR/mac-sync]
#             Output: $X0X_SYNC_DIR/<YYYYmmdd-HHMMSS>.tar and the symlink latest.tar to it.
#   --unpack: X0X_RESULTS_DIR [$HOME/Documents/x0x37/gate-results]
#             The logs go to $X0X_RESULTS_DIR/logs/<box logs path>. This agrees with
#             X0X_GATE_LOGS_MAP="/=$X0X_RESULTS_DIR/logs/" (require_gate_pass.sh also finds them there itself).
#             All files that it writes are made read-only (chmod a-w). It deletes no file.
# Runs with bash 3.2 (macOS) and Linux.
set -eu
usage() { echo "usage: $0 --pack | --unpack <tar>" >&2; exit 2; }
if command -v sha256sum >/dev/null 2>&1; then SUM="sha256sum"; else SUM="shasum -a 256"; fi
case "${1:-}" in
--pack)
    [ $# -eq 1 ] || usage
    RES=${X0X_RESULTS_DIR:-/workspace/x0x37-results}
    OUT=${X0X_SYNC_DIR:-$RES/mac-sync}
    [ -f "$RES/index.md" ] || { echo "error: no $RES/index.md" >&2; exit 1; }
    ST=$(mktemp -d "${TMPDIR:-/tmp}/x0xsync.XXXXXX"); trap 'rm -rf "$ST"' EXIT
    G="$ST/gate-results"; mkdir -p "$G/logs"
    cp "$RES/index.md" "$G/"
    [ -f "$RES/QUEUE.md" ] && cp "$RES/QUEUE.md" "$G/"
    NR=0; NL=0
    # result files: column 6 of the index rows (first word), only plain file names
    for f in $(awk -F '|' 'NF > 6 { f=$6; gsub(/^ +| +$/, "", f); split(f, a, " "); if (a[1] ~ /^[0-9a-f]+[-.a-z0-9]*\.md$/) print a[1] }' "$RES/index.md" | sort -u); do
        [ -f "$RES/$f" ] || continue
        cp "$RES/$f" "$G/"; NR=$((NR + 1))
        L=$(sed -n 's/^- Worktree: .*\. Logs: \(.*\)\.$/\1/p' "$RES/$f" | head -1)
        case "$L" in /*) ;; *) continue ;; esac
        case "$L" in *..*) continue ;; esac
        [ -d "$L" ] || continue
        [ -d "$G/logs$L" ] && continue
        mkdir -p "$G/logs$L"
        for n in gate.log crc.txt result.md validation-inputs.json; do [ -f "$L/$n" ] && cp "$L/$n" "$G/logs$L/"; done
        NL=$((NL + 1))
    done
    (cd "$G" && find . -type f ! -name SHA256SUMS | sed 's|^\./||' | sort | while read -r p; do $SUM "$p"; done > SHA256SUMS)
    mkdir -p "$OUT"
    TS=$(date +%Y%m%d-%H%M%S)
    tar -C "$ST" -cf "$OUT/$TS.tar.part" gate-results
    mv "$OUT/$TS.tar.part" "$OUT/$TS.tar"
    ln -sfn "$TS.tar" "$OUT/latest.tar"
    echo "packed $OUT/$TS.tar ($NR result files, $NL logs folders); latest.tar -> $TS.tar"
    ;;
--unpack)
    [ $# -eq 2 ] || usage
    TAR=$2; [ -f "$TAR" ] || { echo "error: no file $TAR" >&2; exit 1; }
    DST=${X0X_RESULTS_DIR:-$HOME/Documents/x0x37/gate-results}
    # refuse absolute paths, ".." and paths outside gate-results/
    if tar -tf "$TAR" | grep -vE '^gate-results(/|$)' | grep -q .; then echo "error: $TAR has paths outside gate-results/" >&2; exit 1; fi
    if tar -tf "$TAR" | grep -qE '(^|/)\.\.(/|$)'; then echo "error: $TAR has .. paths" >&2; exit 1; fi
    mkdir -p "$DST"
    ST=$(mktemp -d "$DST/.unpack.XXXXXX"); trap 'chmod -R u+w "$ST" 2>/dev/null; rm -rf "$ST"' EXIT
    tar -C "$ST" -xf "$TAR"
    S="$ST/gate-results"
    [ -f "$S/SHA256SUMS" ] && [ -f "$S/index.md" ] || { echo "error: $TAR is not a gate results pack" >&2; exit 1; }
    (cd "$S" && $SUM -c --quiet SHA256SUMS >/dev/null 2>&1 || $SUM -c SHA256SUMS >/dev/null) || { echo "error: checksum failure in $TAR" >&2; exit 1; }
    N=0
    while read -r _ p; do
        d=$(dirname "$DST/$p"); mkdir -p "$d"
        if [ -f "$DST/$p" ] && cmp -s "$S/$p" "$DST/$p"; then chmod a-w "$DST/$p"; continue; fi
        [ -f "$DST/$p" ] && chmod u+w "$DST/$p"
        cp "$S/$p" "$DST/$p.new.$$" && mv -f "$DST/$p.new.$$" "$DST/$p"
        chmod a-w "$DST/$p"; N=$((N + 1))
    done < "$S/SHA256SUMS"
    echo "unpacked $TAR into $DST ($N files changed, all read-only); use X0X_RESULTS_DIR=$DST X0X_GATE_LOGS_MAP=/=$DST/logs/"
    ;;
*) usage ;;
esac
