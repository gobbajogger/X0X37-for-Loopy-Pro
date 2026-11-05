#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# D6: PART_OFF_WEB_E2E_FORCE_NOT_RUN prints NOT_RUN for a required part-off web check.
# run_tests.sh would count exit 0 as ok. This test fails if that skip exits 0.
set -eu
cd "$(dirname "$0")/../.."
rc=0
out=$(PART_OFF_WEB_E2E_FORCE_NOT_RUN=forced /bin/sh tests/host/run_part_off_web_e2e.sh 2>&1) || rc=$?
printf '%s\n' "$out"
case "$out" in
    *"NOT_RUN part_off_web_e2e (forced)"*) ;;
    *) echo "  FAIL not_run_exit (expected NOT_RUN line)"; exit 1 ;;
esac
if [ "$rc" -eq 0 ]; then
    echo "  FAIL not_run_exit (exit 0 is false-green; want non-zero)"
    exit 1
fi
echo "  ok   part_off_web_e2e_not_run_exit (NOT_RUN => exit $rc)"
