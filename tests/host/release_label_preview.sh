#!/bin/sh
# Draw the GLOBAL > ABOUT row in the headless host simulator and check the label fits.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
HOST_DEFS=
if [ "$(uname -s)" = Darwin ]; then HOST_DEFS=-D_DARWIN_C_SOURCE; fi
X0X_HOST_OUT="$OUT/host" X0X_HOST_DEFS="$HOST_DEFS" sh host/build_host.sh --generate-only > "$OUT/build.log" 2>&1 || {
    cat "$OUT/build.log"
    exit 1
}
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c"
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST $HOST_DEFS -DX0X_VERSION=\"0.1.3\" -DX0X_RELEASE_LABEL=\"0.1.3_openbeta\" \
    -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/release_label_preview.c $U -lm -o "$OUT/preview"
"$OUT/preview" "$OUT/about-row.png"
cp "$OUT/about-row.png" "${1:-/tmp/about-row.png}"
echo "headless preview: ${1:-/tmp/about-row.png}"
