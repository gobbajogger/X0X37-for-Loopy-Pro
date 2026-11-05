#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host keyboard and SMC panic routes. Device and native runs are not this script.
set -eu
cd "$(dirname "$0")/../.."
out=$(mktemp -d "${TMPDIR:-/tmp}/x0x-panic-route.XXXXXX")
trap 'rm -rf "$out"' EXIT HUP INT TERM
if [ ! -f build/gen/felucca_font.h ] || [ ! -f build/gen/x0x_drum_samples.h ] || \
   [ ! -f build/gen/x0x_drum_tables.h ] || [ ! -f build/gen/x0x_break_bank.h ]; then
    sh host/build_host.sh --generate-only
fi
units="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/bass303.c firmware/src/dsp/breaks.c firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c experiments/wobble/wobble.c"
host_defs=
if [ "$(uname -s)" = Darwin ]; then host_defs=-D_DARWIN_C_SOURCE; fi
san=
if "${CC:-cc}" -fsanitize=address,undefined -x c -o "$out/san-probe" - <<'EOF' >/dev/null 2>&1
int main(void) { return 0; }
EOF
then
    san="-fsanitize=address,undefined -fno-sanitize-recover=all"
else
    echo "panic route sanitizer: NOT_RUN"
fi
# shellcheck disable=SC2086
"${CC:-cc}" -O1 -g $san -ffp-contract=off -std=c99 \
    -Wno-unused-function -Wno-unused-parameter -DX0X_HOST -DBOARD_SMK37=1 \
    -DX0X_WOBBLE=1 -DX0X_DEV_WOBBLE=1 \
    $host_defs -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/panic_route_test.c $units -lm -o "$out/route"
"$out/route"
