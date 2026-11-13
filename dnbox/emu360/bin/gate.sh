#!/usr/bin/env bash
# Official-style DnBOX emu360 gate.
# The historical helper was /workspace/zdprobe/dnbox360/gate.sh (not on this box).
# PASS = x0x_dbg.max_us < 11609 and x0x_dbg.late == 0 (firmware audio-half accounting).
#
# Usage: gate.sh --fwsc PACKAGE.fwsc --elf APP.elf [--shot MS] [--bin fm1-live] [--out DIR]
set -euo pipefail
SHOT_MS=8000
BIN=""
FWSC=""
ELF=""
OUT=""
MHZ=360
LIMIT_US=11609
while [ $# -gt 0 ]; do
  case "$1" in
    --fwsc) FWSC="$2"; shift 2 ;;
    --elf) ELF="$2"; shift 2 ;;
    --shot) SHOT_MS="$2"; shift 2 ;;
    --bin) BIN="$2"; shift 2 ;;
    --out) OUT="$2"; shift 2 ;;
    --mhz) MHZ="$2"; shift 2 ;;
    -h|--help) sed -n '2,8p' "$0"; exit 0 ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done
[ -n "$FWSC" ] && [ -f "$FWSC" ] || { echo "gate.sh: --fwsc required" >&2; exit 2; }
[ -n "$ELF" ] && [ -f "$ELF" ] || { echo "gate.sh: --elf required" >&2; exit 2; }
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="${ROOT:-${EMU360_ROOT:-$HERE}}"
if [ -z "$BIN" ]; then
  if [ -n "${FM1_LIVE:-}" ]; then
    BIN="$FM1_LIVE"
  elif command -v fm1-live >/dev/null 2>&1; then
    BIN="$(command -v fm1-live)"
  else
    BIN="$ROOT/X0X37-for-Loopy-Pro-8c8fa82b789d7dabde04d2fa2334b5616becda47/target/release/fm1-live"
  fi
fi
[ -x "$BIN" ] || { echo "gate.sh: no fm1-live at $BIN" >&2; exit 2; }

range=$(nm "$ELF" 2>/dev/null | awk '
  $3=="_c1_start" { s=$1 }
  $3=="_c1_end" { e=$1 }
  END { if (s=="" || e=="") exit 1; print s "-" e }
') || { echo "gate.sh: ELF $ELF has no _c1_start/_c1_end" >&2; exit 2; }

dbg_addr=$(nm "$ELF" 2>/dev/null | awk '$3=="x0x_dbg"{print $1; exit}')
[ -n "$dbg_addr" ] || { echo "gate.sh: ELF $ELF has no x0x_dbg" >&2; exit 2; }

if [ -z "$OUT" ]; then
  OUT=$(mktemp -d /tmp/emu360-gate.XXXXXX)
else
  mkdir -p "$OUT"
fi
csv="$OUT/core_load.csv"
ram="$OUT/ram.bin"
log="$OUT/fm1-live.log"

# Default --shot script (fm1-live): C4-E4-G4 held from 800 ms, PLAY at 1200-1350 ms.
# 8000 ms is the catalog duration in the emu repo (PLAN.md).
# FM1_STRICT=1: device-accurate cycle costs, interpreter, timed LCD.
# --mhz 360 is also fm1-live's default after the 360 MHz patch.
set +e
env FM1_STRICT=1 \
    FM1_CORE_LOAD="$range" \
    FM1_CORE_LOAD_CSV="$csv" \
    FM1_DUMPRAM="$ram" \
    "$BIN" --fwsc "$FWSC" --mhz "$MHZ" --shot "$SHOT_MS" \
    >"$log" 2>&1
ex=$?
set -e
if [ $ex -ne 0 ]; then
  echo "gate.sh: fm1-live exited $ex" >&2
  tail -50 "$log" >&2
  exit $ex
fi
python3 - "$ram" "$dbg_addr" "$csv" "$LIMIT_US" "$OUT" <<'PY'
import sys, struct, csv, pathlib
ram_path, dbg_hex, csv_path, limit, out = sys.argv[1:6]
limit = float(limit)
out = pathlib.Path(out)
RAM_BASE = 0x01C00000
dbg_addr = int(dbg_hex, 16)
data = pathlib.Path(ram_path).read_bytes()
off = dbg_addr - RAM_BASE
# struct x0x_dbg { magic, halves, max_us, nested, in_audio, late, timer_irqs, ui_frames,
#                  last_us, cpu_q8, boots, stage, crash_seen }
fields = ("magic","halves","max_us","nested","in_audio","late","timer_irqs","ui_frames",
          "last_us","cpu_q8","boots","stage","crash_seen")
vals = struct.unpack_from("<"+"I"*len(fields), data, off)
dbg = dict(zip(fields, vals))
if dbg["magic"] != 0x44424731:
    print(f"gate.sh: bad x0x_dbg magic 0x{dbg['magic']:08x} at 0x{dbg_addr:08x}", file=sys.stderr)
    sys.exit(2)
# supplementary CORE_LOAD (core1 time outside .c1_text, per audio half)
BIN_US = 512.0 * 1_000_000.0 / 44100.0
cl_worst = 0.0
cl_late = 0
cl_bins = 0
p = pathlib.Path(csv_path)
if p.is_file():
    rows = list(csv.DictReader(p.open()))
    full = [int(r["core1_ticks"]) for r in rows if int(r["core1_ticks"]) > 0]
    thresh = 0.8 * max(full) if full else 0
    for r in rows:
        tot = int(r["core1_ticks"]); busy = int(r["core1_busy_ticks"])
        if tot < thresh:
            continue
        us = busy / tot * BIN_US
        cl_bins += 1
        if us > cl_worst: cl_worst = us
        if us >= limit: cl_late += 1

max_us = dbg["max_us"]
late = dbg["late"]
passed = (max_us < limit) and (late == 0) and (dbg["halves"] > 0)
summary = (
    f"result={'PASS' if passed else 'FAIL'} max_us={max_us} late={late} "
    f"halves={dbg['halves']} last_us={dbg['last_us']} limit_us={int(limit)} "
    f"core_load_worst_us={cl_worst:.1f} core_load_late={cl_late} core_load_bins={cl_bins}"
)
print(summary)
(out/"summary.txt").write_text(summary+"\n"+repr(dbg)+"\n")
sys.exit(0 if passed else 1)
PY
