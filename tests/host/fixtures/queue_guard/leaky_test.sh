#!/bin/sh
# Fixture for the scan in tests/host/test_queue_guard.sh: a BAD test. It runs tools/require_gate_pass.sh with no
# temp queue file, so the guard reads the live QUEUE.md. The scan must flag this file. Never run it.
G="$(cd "$(dirname "$0")/../../../.." && pwd)/tools/require_gate_pass.sh"
X0X_RESULTS_DIR=/tmp/none bash "$G" HEAD
