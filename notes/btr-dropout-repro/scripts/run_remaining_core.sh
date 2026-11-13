#!/bin/bash
set -euo pipefail
ROOT=/workspace/btr-dropout-repro
LOG=$ROOT/logs/core-remaining-launch.log
exec > >(tee -a "$LOG") 2>&1
echo "==== remaining core start $(date) ===="
cfgs=(full-fx-allon loo-no-arp loo-no-juno loo-no-909)
for cfg in "${cfgs[@]}"; do
  FREE=$(df -B1 / | awk 'NR==2{print $4}')
  echo "BEFORE $cfg disk_free=$FREE $(date)"
  if [ "$FREE" -lt $((2*1024*1024*1024)) ]; then
    echo "STOP: disk free $FREE under 2 GiB; not launching $cfg"
    exit 3
  fi
  if pgrep -f 'fm1-live --fwsc' >/dev/null; then
    echo "STOP: fm1-live already running"
    pgrep -af fm1-live
    exit 4
  fi
  python3 "$ROOT/scripts/run_core_config.py" "$cfg"
  python3 "$ROOT/scripts/hash_core_logs.py" "$ROOT/logs/core/$cfg"
  # copy meaning if present
  if [ -f "$ROOT/keys/${cfg}.MEANING.txt" ]; then
    cp "$ROOT/keys/${cfg}.MEANING.txt" "$ROOT/logs/core/$cfg/MEANING.txt"
  fi
  echo "AFTER $cfg $(date) disk=$(df -h / | awk 'NR==2{print $4}')"
done
python3 "$ROOT/scripts/write_core_summary.py"
echo "==== remaining core all done $(date) ===="
