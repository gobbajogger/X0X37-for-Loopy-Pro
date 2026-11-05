#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Negative gate for run_part_off_web_e2e.sh: a PATH without node must FAIL (non-zero),
# not exit 0 / NOT_RUN (D6: no false-green admission).
set -eu
cd "$(dirname "$0")/../.."
GATE_BIN=$(mktemp -d)
trap 'rm -rf "$GATE_BIN"' EXIT
# Minimal tools the launcher needs before the node check; deliberately omit node.
for t in sh basename dirname echo cat; do
    src=$(command -v "$t") || { echo "  FAIL node_gate setup (need $t)"; exit 1; }
    ln -s "$src" "$GATE_BIN/$t"
done
if PATH="$GATE_BIN" command -v node >/dev/null 2>&1; then
    echo "  FAIL node_gate setup (node still visible)"
    exit 1
fi
rc=0
out=$(PATH="$GATE_BIN" /bin/sh tests/host/run_part_off_web_e2e.sh 2>&1) || rc=$?
printf '%s\n' "$out"
case "$out" in
    *"FAIL part_off_web_e2e (node not found)"*) ;;
    *) echo "  FAIL node_gate (expected FAIL node-not-found message)"; exit 1 ;;
esac
if [ "$rc" -eq 0 ]; then
    echo "  FAIL node_gate (exit 0 is false-green; want non-zero)"
    exit 1
fi
echo "  ok   part_off_web_e2e_node_gate (missing node => exit $rc)"
