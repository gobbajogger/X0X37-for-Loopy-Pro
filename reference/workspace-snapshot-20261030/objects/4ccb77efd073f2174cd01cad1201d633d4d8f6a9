#!/bin/bash
# Fixture for check 30 (tests/host/test_status_isolation.sh): a BAD status test.
# It runs tools/status.sh with no output folder, so status.sh would write the live orchestrator folder.
# The isolation test must flag this file. Never run it.
TOP="$(cd "$(dirname "$0")/../../../.." && pwd)"
X0X_REPO=/tmp/none bash "$TOP/tools/status.sh"
