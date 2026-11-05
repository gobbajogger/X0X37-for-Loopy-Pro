#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build the X0X host simulator (the whole app on this machine) into build/host/x0x_host.
# Use --generate-only to prepare assets without compiling the simulator.
# Same sources as the firmware, -ffp-contract=off like the device (no fused multiply-add).
set -e
cd "$(dirname "$0")/.."
case "${1:-}" in
    ""|--generate-only) ;;
    *) echo "usage: $0 [--generate-only]" >&2; exit 2 ;;
esac
CC="${CC:-cc}"
mkdir -p build/gen build/host
[ -f build/gen/felucca_font.h ] || python3 tools/gen_font.py build/gen/felucca_font.h >/dev/null
# Regen when samples or tables are missing, or when the packing generator is newer
# (EXP pot helper lives here). Tables alone can vanish while samples stay; both
# come from the same script, so either miss must re-run it before break-bank compile.
if [ ! -f build/gen/x0x_drum_samples.h ] || [ ! -f build/gen/x0x_drum_tables.h ] \
    || [ tools/gen_drum_samples.py -nt build/gen/x0x_drum_samples.h ]; then
    python3 tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
fi
if [ -n "${X0X_BREAK_BANK:-}" ]; then python3 tools/gen_break_bank.py "$X0X_BREAK_BANK" build/gen/x0x_break_bank.h >/dev/null
else sh tools/gen_builtin_break.sh build/gen >/dev/null; fi
[ "${1:-}" != --generate-only ] || exit 0
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c
   firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
   firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c
   firmware/src/app/factory_banks.c"
# Only Chicago and Miami ship (the bass303 engine is removed). Both need the
# private original-synth inputs; there is no public-safe fallback.
if ! python3 tools/check_original_inputs.py >/dev/null 2>&1; then
    echo 'host: ERROR: private original-synth inputs missing (Chicago and Miami need them).' >&2
    echo 'host: run python3 tools/check_original_inputs.py for the list; no public fallback exists.' >&2
    exit 2
fi
case "${X0X_ORIGINAL_SYNTH:-1}" in
    1) DEV_WOBBLE="${X0X_DEV_WOBBLE:-1}"
       case "$DEV_WOBBLE" in 0|1) ;; *) echo 'X0X_DEV_WOBBLE: 0 or 1' >&2; exit 2 ;; esac
       U="$U $(python3 - <<'PY'
import importlib.util
spec = importlib.util.spec_from_file_location('x0x_build', 'tools/build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
build.synth_build_config()
build.require_original_synth_sources()
print(' '.join([*build.ORIGINAL_SYNTH_UNITS, *build.ORIGINAL_SYNTH_BACKENDS]))
PY
)"
       SYNTH_DEFS="-DX0X_ORIGINAL_SYNTH=1 -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=$DEV_WOBBLE -fno-fast-math -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -Iexperiments/original_synth -Iexperiments/wobble" ;;
    *) echo 'X0X_ORIGINAL_SYNTH: only 1 is supported (bass303 is removed)' >&2; exit 2 ;;
esac
# shellcheck disable=SC2086
$CC -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST $SYNTH_DEFS ${X0X_HOST_DEFS:-} -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    -o ${X0X_HOST_OUT:-build/host/x0x_host} host/x0x_host.c $U -lm
echo "host: build/host/x0x_host"
