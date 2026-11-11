#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host engine check for M1 note clearing. Device and native runs are not this script.
set -eu
cd "$(dirname "$0")/../.."
out=$(mktemp -d "${TMPDIR:-/tmp}/x0x-panic-clear.XXXXXX")
trap 'rm -rf "$out"' EXIT HUP INT TERM
if [ ! -f build/gen/felucca_font.h ] || [ ! -f build/gen/x0x_drum_samples.h ] || \
   [ ! -f build/gen/x0x_drum_tables.h ] || [ ! -f build/gen/x0x_break_bank.h ]; then
    sh host/build_host.sh --generate-only
fi
units=$(python3 - <<'PYUNITS'
import importlib.util
spec=importlib.util.spec_from_file_location('build','tools/build.py')
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
print(' '.join(['firmware/src/'+u for u in b.O2_UNITS]+['firmware/src/app/factory_banks.c',*b.ORIGINAL_SYNTH_UNITS,*b.ORIGINAL_SYNTH_BACKENDS]))
PYUNITS
)
host_defs=
if [ "$(uname -s)" = Darwin ]; then host_defs=-D_DARWIN_C_SOURCE; fi
san=
if "${CC:-cc}" -fsanitize=address,undefined -x c -o "$out/san-probe" - <<'EOF' >/dev/null 2>&1
int main(void) { return 0; }
EOF
then
    san="-fsanitize=address,undefined -fno-sanitize-recover=all"
else
    echo "panic clear sanitizer: NOT_RUN"
fi
for wobble in 1 0; do
    # shellcheck disable=SC2086
    "${CC:-cc}" -O1 -g $san -ffp-contract=off -std=c99 \
        -Wno-unused-function -Wno-unused-parameter -DX0X_HOST -DX0X_ORIGINAL_SYNTH=1 -DX0X_WOBBLE=0 -DX0X_DEV_WOBBLE=$wobble \
        $host_defs -DORIGINAL_USE_SOFT_FMA -DMIAMI_FX_NO_HOST_FENV -fno-fast-math -Iexperiments/original_synth -Iexperiments/wobble -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
        tests/host/panic_clear_test.c $units -lm -o "$out/clear-$wobble"
    "$out/clear-$wobble"
done
