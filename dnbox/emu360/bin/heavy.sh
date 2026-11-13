#!/usr/bin/env bash
# Heavy-load emu360 gate for DnBOX firmware (written; official scene not found).
#
# Usage: bin/heavy.sh elite|pro [fwsc]
# Optional env: ELF, FLASH, BIN, ROOT, SHOT_MS (8000), BOOT_MS (2000),
#               KEYS, OUT, LIMIT_US (11609), SKIP_BOOT=1
#
# Prints max_us, late (raw and during-test), PASS/FAIL.
# PASS = max_us < 11609 AND late_during_test == 0.
# late_during_test = late_heavy - late_boot, because a prepared-flash boot on
# this box systematically records late=1 in the default (light) scene; the
# official helper was not found, but PR #863 says "0 late during the test".
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="${ROOT:-${EMU360_ROOT:-$HERE}}"
# shellcheck disable=SC1091
[ -f "$ROOT/env.sh" ] && . "$ROOT/env.sh"

MODEL="${1:-}"
case "$MODEL" in
  elite|pro) ;;
  -h|--help|"")
    sed -n '2,14p' "$0"
    [ -n "$MODEL" ] && exit 0
    exit 2
    ;;
  *) echo "heavy.sh: first arg must be elite or pro" >&2; exit 2 ;;
esac

SRC25="$ROOT/X0X37-for-Loopy-Pro-78f81bad07b8e53161326c056a2111c663349772"
EMU="$ROOT/X0X37-for-Loopy-Pro-8c8fa82b789d7dabde04d2fa2334b5616becda47"
BIN="${BIN:-$EMU/target/release/fm1-live}"
KEYS="${KEYS:-$ROOT/scenes/heavy.keys}"
SHOT_MS="${SHOT_MS:-8000}"
BOOT_MS="${BOOT_MS:-2000}"
LIMIT_US="${LIMIT_US:-11609}"
MHZ="${MHZ:-360}"

if [ "$MODEL" = elite ]; then
  FWSC="${2:-$SRC25/build/x0x-smk37-elite.fwsc}"
  ELF="${ELF:-$SRC25/build/smk37-elite/x0x.elf}"
  FLASH_SRC="${FLASH:-$ROOT/out/elite-25.1.flash}"
else
  FWSC="${2:-$SRC25/build/x0x-smk37.fwsc}"
  ELF="${ELF:-$SRC25/build/smk37/x0x.elf}"
  FLASH_SRC="${FLASH:-$ROOT/out/pro-25.1.flash}"
fi

[ -x "$BIN" ] || { echo "heavy.sh: no fm1-live at $BIN" >&2; exit 2; }
[ -f "$FWSC" ] || { echo "heavy.sh: no package $FWSC" >&2; exit 2; }
[ -f "$ELF" ] || { echo "heavy.sh: no ELF $ELF" >&2; exit 2; }
[ -f "$KEYS" ] || { echo "heavy.sh: no keys $KEYS (run scenes/gen_heavy.py)" >&2; exit 2; }

range=$(nm "$ELF" 2>/dev/null | awk '
  $3=="_c1_start" { s=$1 }
  $3=="_c1_end" { e=$1 }
  END { if (s=="" || e=="") exit 1; print s "-" e }
') || { echo "heavy.sh: ELF $ELF has no _c1_start/_c1_end" >&2; exit 2; }
dbg_addr=$(nm "$ELF" 2>/dev/null | awk '$3=="x0x_dbg"{print $1; exit}')
[ -n "$dbg_addr" ] || { echo "heavy.sh: ELF $ELF has no x0x_dbg" >&2; exit 2; }

stamp=$(date +%Y%m%d-%H%M%S)
OUT="${OUT:-$ROOT/out/heavy/${MODEL}-${stamp}}"
mkdir -p "$OUT" "$OUT/work"
# Isolated work/ so panel.ppm dumps succeed
run_one() {
  local label="$1" ms="$2" dest="$3"
  mkdir -p "$dest"
  local flash="$dest/flash.bin"
  if [ -f "$FLASH_SRC" ]; then
    cp -f "$FLASH_SRC" "$flash"
  else
    echo "heavy.sh: warning: no prepared flash at $FLASH_SRC; firmware will FORMAT" >&2
    : > "$flash" || true
    rm -f "$flash"
  fi
  local t0 t1
  t0=$(date +%s.%N)
  set +e
  env FM1_STRICT=1 \
      FM1_BENCH_KEYS="@$KEYS" \
      FM1_CORE_LOAD="$range" \
      FM1_CORE_LOAD_CSV="$dest/core_load.csv" \
      FM1_DUMPRAM="$dest/ram.bin" \
      "$BIN" --fwsc "$FWSC" --mhz "$MHZ" --shot "$ms" \
        ${flash:+--flash "$flash"} \
      >"$dest/fm1-live.log" 2>&1
  local ex=$?
  set -e
  t1=$(date +%s.%N)
  python3 - "$t0" "$t1" <<'PY' >"$dest/wall.txt"
import sys
print(f"{float(sys.argv[2])-float(sys.argv[1]):.3f}")
PY
  if [ $ex -ne 0 ]; then
    echo "heavy.sh: $label fm1-live exited $ex" >&2
    tail -40 "$dest/fm1-live.log" >&2
    exit $ex
  fi
  python3 - "$dest/ram.bin" "$dbg_addr" "$dest" "$LIMIT_US" "$label" <<'PY'
import sys, struct, pathlib
ram_path, dbg_hex, dest, limit, label = sys.argv[1:6]
limit = float(limit)
dest = pathlib.Path(dest)
RAM_BASE = 0x01C00000
dbg_addr = int(dbg_hex, 16)
data = pathlib.Path(ram_path).read_bytes()
off = dbg_addr - RAM_BASE
fields = ("magic","halves","max_us","nested","in_audio","late","timer_irqs","ui_frames",
          "last_us","cpu_q8","boots","stage","crash_seen")
vals = struct.unpack_from("<"+"I"*len(fields), data, off)
dbg = dict(zip(fields, vals))
if dbg["magic"] != 0x44424731:
    print(f"heavy.sh: bad x0x_dbg magic 0x{dbg['magic']:08x}", file=sys.stderr)
    sys.exit(2)
# audio peak from the log, if present
peak = ""
log = dest/"fm1-live.log"
if log.is_file():
    for line in log.read_text(errors="replace").splitlines():
        if "peak" in line.lower() or "audio" in line.lower():
            peak = line.strip()
text = (
    f"label={label} max_us={dbg['max_us']} late={dbg['late']} "
    f"halves={dbg['halves']} last_us={dbg['last_us']} cpu_q8={dbg['cpu_q8']} "
    f"limit_us={int(limit)} nested={dbg['nested']}"
)
print(text)
(dest/"summary.txt").write_text(text+"\n"+repr(dbg)+"\n"+peak+"\n")
(dest/"dbg.txt").write_text(
    f"max_us={dbg['max_us']}\nlate={dbg['late']}\nhalves={dbg['halves']}\n"
    f"last_us={dbg['last_us']}\ncpu_q8={dbg['cpu_q8']}\n"
)
PY
}

echo "heavy.sh model=$MODEL fwsc=$FWSC elf=$ELF keys=$KEYS shot=${SHOT_MS}ms boot=${BOOT_MS}ms" 
echo "out=$OUT"

boot_max=0
boot_late=0
if [ "${SKIP_BOOT:-0}" != 1 ]; then
  echo "--- boot baseline ${BOOT_MS} ms (same script; events start after ${BOOT_MS} ms) ---"
  run_one boot "$BOOT_MS" "$OUT/boot"
  cat "$OUT/boot/summary.txt"
  boot_max=$(awk -F= '/^max_us=/{print $2}' "$OUT/boot/dbg.txt")
  boot_late=$(awk -F= '/^late=/{print $2}' "$OUT/boot/dbg.txt")
fi

echo "--- heavy shot ${SHOT_MS} ms ---"
# run from $OUT so work/panel.ppm can be written
(
  cd "$OUT"
  # run_one uses absolute dest; panel.ppm wants cwd/work
  run_one heavy "$SHOT_MS" "$OUT/heavy"
)
cat "$OUT/heavy/summary.txt"

python3 - "$MODEL" "$OUT" "$LIMIT_US" "$boot_max" "$boot_late" "$SHOT_MS" "$BOOT_MS" <<'PY'
import pathlib, sys
model, out, limit, boot_max, boot_late, shot, boot_ms = sys.argv[1:8]
limit = int(float(limit))
boot_max = int(boot_max)
boot_late = int(boot_late)
out = pathlib.Path(out)
dbg = {}
for line in (out/"heavy/dbg.txt").read_text().splitlines():
    k, _, v = line.partition("=")
    dbg[k] = int(v) if v.lstrip("-").isdigit() else v
wall_h = (out/"heavy/wall.txt").read_text().strip()
wall_b = (out/"boot/wall.txt").read_text().strip() if (out/"boot/wall.txt").is_file() else "0"
late = dbg["late"]
max_us = dbg["max_us"]
late_test = max(0, late - boot_late)
# If the heavy peak is no higher than boot, the scene did not load the audio half.
passed = (max_us < limit) and (late_test == 0) and (dbg["halves"] > 0)
verdict = "PASS" if passed else "FAIL"
lines = [
    f"result={verdict} model={model} max_us={max_us} late={late} late_during_test={late_test} late_boot={boot_late}",
    f"boot_max_us={boot_max} halves={dbg['halves']} last_us={dbg['last_us']} cpu_q8={dbg['cpu_q8']} limit_us={limit}",
    f"shot_ms={shot} boot_ms={boot_ms} wall_heavy_s={wall_h} wall_boot_s={wall_b}",
    f"pass_rule=max_us<{limit} and late_during_test==0 (startup late ignored via boot subtraction)",
]
text = "\n".join(lines) + "\n"
print(text, end="")
(out/"RESULT.txt").write_text(text)
sys.exit(0 if passed else 1)
PY
