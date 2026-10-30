#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Deterministic, read-only web and sample-safety checks. No firmware build,
# USB/MIDI device access, package publication, or source mutation.
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."
mkdir -p build/bugfinder
run_dir="$(mktemp -d build/bugfinder/run.XXXXXXXX)"
mkdir -p "$run_dir/logs"
summary="$run_dir/summary.md"
passed=0
failed=0
source_commit="$(git rev-parse --verify HEAD 2>/dev/null || echo unknown)"

printf '## LoopyX0X37 local smoke checks\n\n' > "$summary"
printf 'Checked Git source: `%s`\n\n' "$source_commit" >> "$summary"
printf '| Check | Result |\n| --- | --- |\n' >> "$summary"

check() {
  local name="$1"; shift
  local logfile="$run_dir/logs/${name}.log"
  if "$@" > "$logfile" 2>&1; then
    passed=$((passed + 1))
    printf '| `%s` | PASS |\n' "$name" >> "$summary"
    printf 'PASS %s\n' "$name"
  else
    local code=$?
    failed=$((failed + 1))
    printf '| `%s` | FAIL (exit %s) |\n' "$name" "$code" >> "$summary"
    printf 'FAIL %s (exit %s)\n' "$name" "$code"
    tail -n 15 "$logfile" || true
  fi
}

check js-breaks-syntax node --check web/x0x_breaks.js
check js-projects-syntax node --check web/x0x_projects.js
check breaks-web node web/test_breaks.mjs
check projects-web node web/test_projects.mjs
check projects-page node web/test_projects_page.mjs
check slot-backup python3 tests/x0x_slots_test.py
check installer-slots python3 tests/install_slots_test.py

printf '\nResults: %s passed; %s failed. Host checks only, no firmware or hardware PASS.\n' "$passed" "$failed" >> "$summary"
if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  cat "$summary" >> "$GITHUB_STEP_SUMMARY"
fi
printf 'Summary and logs: %s\n' "$run_dir"
printf 'Check summary: %s passed; %s failed\n' "$passed" "$failed"
if (( failed > 0 )); then
  exit 1
fi
