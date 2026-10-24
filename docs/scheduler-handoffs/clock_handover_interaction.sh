#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p logs
: > logs/summary.txt
for cc in gcc clang; do
 for mode in CANDIDATE DROP_ONLY TAKEOVER_ONLY COMBINED; do
  defs=''
  case "$mode" in DROP_ONLY) defs='-DHANDOVER_DROP';; TAKEOVER_ONLY) defs='-DSTOP_TAKEOVER';; COMBINED) defs='-DHANDOVER_DROP -DSTOP_TAKEOVER';; esac
  exe="logs/${cc}_${mode}"
  # shellcheck disable=SC2086
  "$cc" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all $defs clock_handover_interaction.c -o "$exe" > "$exe.compile.log" 2>&1
  set +e
  "$exe" > "$exe.execute.log" 2>&1; rc=$?
  set -e
  summary=$(grep '^RESULT ' "$exe.execute.log")
  echo "$cc $mode compile=0 run=$rc $summary" | tee -a logs/summary.txt
  case "$mode" in COMBINED) test "$rc" -eq 0;; *) test "$rc" -eq 1;; esac
 done
done
