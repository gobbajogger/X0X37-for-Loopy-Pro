#!/bin/sh
# Fixture for tests/host/test_gate_trap.sh (check 5): a child of tools/gate_sigwrap.py. On SIGTERM it waits for the
# sender line in X0X_GATE_SIGFILE, prints it and exits with 143.
trap 'i=0; while [ ! -s "$X0X_GATE_SIGFILE" ] && [ $i -lt 20 ]; do sleep 0.1; i=$((i + 1)); done; echo "INNER GOT $(cat "$X0X_GATE_SIGFILE" 2>/dev/null)"; exit 143' TERM
echo ready > "$1"
while :; do sleep 0.1; done
