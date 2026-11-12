#!/bin/sh
# Full production UI/project/engine + SMC input/render regression. No device I/O.
# Optional CC, SAN_FLAGS, SMC_UI_OUT and SMC_UI_SOURCE_FILE support independent replay.
set -eu
cd "$(dirname "$0")/../.."
OUT=${SMC_UI_OUT:-build/host/smc-ui}
mkdir -p "$OUT"
awk '/^static const char CK_FLD_N\[\] = /{print}
     /^static uint32_t ck_page_on\(void\)$/ || /^int smk_ck_smc_(turn|nav)\(/{on=1}
     on{print} on && /^}/{on=0}' firmware/src/app/main_fm1.c > "$OUT/clip_smc.inc"
test -s "$OUT/clip_smc.inc"
sh host/build_host.sh --generate-only > "$OUT/generate.log" 2>&1 || { cat "$OUT/generate.log"; exit 1; }
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c
firmware/src/app/factory_banks.c experiments/wobble/wobble.c"
# Shipping-profile opt-in for production mixer UX regression. Legacy fixtures
# keep their existing reference sources; no legacy backend enters the target.
SYNTH_DEFS=
if [ "${SMC_UI_ORIGINAL:-0}" = 1 ]; then
    U="$(printf '%s' "$U" | sed 's@firmware/src/dsp/bass303.c@@; s@experiments/wobble/wobble.c@@') $(python3 - <<'PYUNITS'
import importlib.util
spec = importlib.util.spec_from_file_location('x0x_build', 'tools/build.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
build.require_original_synth_sources()
print(' '.join([*build.ORIGINAL_SYNTH_UNITS, *build.ORIGINAL_SYNTH_BACKENDS]))
PYUNITS
)"
    SYNTH_DEFS='-DX0X_ORIGINAL_SYNTH=1 -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=0 -fno-fast-math -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -Iexperiments/original_synth -Iexperiments/wobble'
fi
DARWIN=
[ "$(uname -s)" != Darwin ] || DARWIN=-D_DARWIN_C_SOURCE
# Linux host needs POSIX clock APIs from x0x_host.c under -std=c99.
POSIX_DEFS=-D_POSIX_C_SOURCE=200809L
SOURCE=${SMC_UI_SOURCE_FILE:-$PWD/firmware/src/app/smc_mixer.c}
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST $SYNTH_DEFS $POSIX_DEFS $DARWIN ${X0X_HOST_DEFS:-} ${SAN_FLAGS:-} \
    -DSMC_SOURCE_FILE=\"$SOURCE\" -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
    ${SMC_UI_TEST_FILE:-tests/host/smc_ui_test.c} $U -lm -o "$OUT/test"
SMC_UI_SHOTS="$OUT" "$OUT/test"
