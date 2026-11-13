#!/usr/bin/env bash
# Elite first-pass runner for the owner dropout scene.
# Does NOT change the live gate. One emu process at a time.
# Does NOT start a 5-minute shot unless FIRST_PASS_VALIDATED=1
# (set that only after a short LCD validation of the key/MIDI script).
set -euo pipefail

ROOT=/workspace/btr-dropout-repro
EMU=/workspace/emu360-target/release/fm1-live
FWSC=/workspace/build-1214/artifacts/elite/DnBOX-0.0.26-SMK37-Elite-94197fc8.fwsc
ELF=/workspace/build-1214/artifacts/elite/x0x.elf
ELF_SHA=720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea
FWSC_SHA=95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6
SRC=94197fc839b00aff47b98ca02f3c6396e4a6eabf
PARSE="$ROOT/scripts/parse_scene.py"
MHZ=360

check_box() {
  echo "---- box ----"
  date
  uptime
  # Step 6 benches: do not run heavy emu if these are live
  if pgrep -af 'node|chrome|chromium' | grep -iE '/workspace/web-emu|web-emu' >/dev/null; then
    echo "BLOCKED: a web-emu Step 6 bench process is running"
    pgrep -af 'node|chrome|chromium' | grep -iE 'web-emu' || true
    return 1
  fi
  echo "no web-emu bench process"
  return 0
}

c1_range() {
  readelf -SW "$ELF" | python3 -c 'import sys
for l in sys.stdin:
    p=l.replace("[ ","[").split()
    if len(p)>5 and p[1]==".c1_text":
        print("%s-%x" % (p[3], int(p[3],16)+int(p[5],16)))'
}

usage() {
  cat <<EOF
usage: $0 validate     # short LCD shot of the scene-drive script (no 5-min)
       $0 run CONFIG   # one first-pass config (needs FIRST_PASS_VALIDATED=1)
       $0 parse DIR    # re-parse a dump

Core CONFIG names (report these first):
  full-fx-default    full scene, FX default, patch 0, default arp, 140 BPM
  full-fx-allon      full scene, FX all-on (chosen meaning, logged)
  loo-no-arp
  loo-no-juno
  loo-no-break       NOT_RUN as a contrast while BREAK cannot play (no 5-min dup)
  loo-no-909

Each run (except loo-no-break): playing >= 5 emulated min, then idle, then
empty sequence (this is also FMB (a): 808 silent, empty seq, no events).

Extras, only after the core report, and only if they stay modest:
  miami-on, miami-off, juno-only
  juno-chorus-off is NOT_RUN (in-DSP chorus would change the sound path).

BREAK=NOT_RUN (pack 131152 B cannot fit in 8440 B tail; no slot loaded).
Owner tempo/patches/FX/installed hash: UNKNOWN.
EOF
}

cmd=${1:-}
if [ -z "$cmd" ] || [ "$cmd" = "-h" ]; then usage; exit 0; fi

# Placeholder: actual key/USB-MIDI scripts are written after the short
# validation designs them. Paths:
#   $ROOT/keys/<config>.keys    FM1_BENCH_KEYS
#   $ROOT/keys/<config>.usb     FM1_USB_SEND
# Validation must prove LCD: CHICAGO+arp, JUNO triad, 909 BD, tempo 140,
# PERF page, then STOP, then empty PLAY.

echo "source $SRC"
echo "elf $ELF sha $ELF_SHA"
echo "fwsc $FWSC sha $FWSC_SHA"
echo "emu $EMU ($($EMU --version 2>&1 | head -1))"
echo "FM1_STRICT=1 mhz=$MHZ BREAK=NOT_RUN"
echo "deadlines: 11609 us budget, ~11598 us I2S half; miss=late and audio_xruns"
check_box || exit 2

case "$cmd" in
  parse)
    python3 "$PARSE" "$ELF" "$2" --fwsc "$FWSC" --elf-sha "$ELF_SHA" --fwsc-sha "$FWSC_SHA" --config "${3:-}" --state "${4:-}"
    ;;
  validate|run)
    if [ "$cmd" = "run" ] && [ "${FIRST_PASS_VALIDATED:-}" != "1" ]; then
      echo "refusing 5-min run: FIRST_PASS_VALIDATED is not 1 (validate the key/MIDI script first)"
      exit 3
    fi
    if [ "$cmd" = "run" ] && [ "${2:-}" = "loo-no-break" ]; then
      echo "loo-no-break: NOT_RUN as a contrast (BREAK already NOT_RUN). Not a duplicate 5-min shot."
      exit 0
    fi
    echo "script paths and shot timings: see $ROOT/keys/ and PHASE1.md"
    echo "not launching: key/MIDI script is not LCD-validated in this tree yet"
    exit 4
    ;;
  *) usage; exit 1 ;;
esac
