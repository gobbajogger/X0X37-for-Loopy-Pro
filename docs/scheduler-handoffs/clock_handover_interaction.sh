#!/bin/sh
set -eu
cd "$(dirname "$0")"
: "${CC_A:=gcc}"; : "${CC_B:=clang}"
mkdir -p threeway-logs
: > threeway-logs/summary.txt
for cc in "$CC_A" "$CC_B"; do
 for mode in CANDIDATE DROP_ONLY TAKEOVER_ONLY OWNER_ONLY DROP_TAKEOVER DROP_OWNER TAKEOVER_OWNER ALL_THREE; do
  defs=''
  case "$mode" in
    DROP_ONLY) defs='-DHANDOVER_DROP';;
    TAKEOVER_ONLY) defs='-DSTOP_TAKEOVER';;
    OWNER_ONLY) defs='-DOWNER_RESET';;
    DROP_TAKEOVER) defs='-DHANDOVER_DROP -DSTOP_TAKEOVER';;
    DROP_OWNER) defs='-DHANDOVER_DROP -DOWNER_RESET';;
    TAKEOVER_OWNER) defs='-DSTOP_TAKEOVER -DOWNER_RESET';;
    ALL_THREE) defs='-DHANDOVER_DROP -DSTOP_TAKEOVER -DOWNER_RESET';;
  esac
  exe="threeway-logs/${cc}_${mode}"
  # shellcheck disable=SC2086
  "$cc" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all $defs clock_handover_interaction.c -o "$exe" > "$exe.compile.log" 2>&1
  set +e
  "$exe" > "$exe.execute.log" 2>&1; rc=$?
  set -e
  summary=$(grep '^RESULT ' "$exe.execute.log")
  echo "$cc $mode compile=0 run=$rc $summary" | tee -a threeway-logs/summary.txt
  case "$mode" in ALL_THREE) test "$rc" -eq 0;; *) test "$rc" -eq 1;; esac
 done
done
