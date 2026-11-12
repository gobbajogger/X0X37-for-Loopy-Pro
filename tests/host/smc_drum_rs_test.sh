#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Production SMC drum R/S, SMK pad-to-pattern routing and audio isolation.
# X0X_SOURCE_ROOT selects an entire alternate source tree for a negative control.
# No target build, radio traffic or physical device is used.
set -eu
TEST_ROOT=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
SRC=${X0X_SOURCE_ROOT:-$TEST_ROOT}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
python3 - "$SRC" "$OUT/smk_pad_route.inc" <<'PY'
from pathlib import Path
import re
import sys
root = Path(sys.argv[1])
pieces = []
for file, name in (("firmware/hal/smk_input.h", "smk__padq"),
                   ("firmware/src/app/main_fm1.c", "smk_grid")):
    source = (root / file).read_text()
    match = re.search(r"^static void " + name + r"\([^\n]*\)[^\n]*\n\{.*?^\}", source, re.M | re.S)
    if not match:
        raise SystemExit("Missing production function: " + name)
    pieces.append(match[0])
masked = "void drum909_render_st_masked" in (root / "firmware/src/dsp/drum909.h").read_text()
Path(sys.argv[2]).write_text(("#define SMC_MASKED_AUDIO\n" if masked else "") + "\n\n".join(pieces) + "\n")
PY
cd "$SRC"
. tests/host/original_build.sh
U=$(printf '%s\n' "$ORIGINAL_SOURCES" | tr ' ' '\n' | grep -v '^firmware/src/app/engine.c$' | tr '\n' ' ')
X0X_HOST_OUT="$OUT/host" sh host/build_host.sh --generate-only > "$OUT/build.log" 2>&1 || { cat "$OUT/build.log"; exit 1; }
# A forwarding observer exposes the real engine's 909 instance to the p-lock
# restoration check. The production DSP setter still performs every write.
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST ${X0X_HOST_DEFS:-} \
    -Ddrum909_set=smc_test_drum909_set -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen $ORIGINAL_FLAGS \
    -c firmware/src/app/engine.c -o "$OUT/engine.o"
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -D_POSIX_C_SOURCE=200809L -DX0X_HOST ${X0X_HOST_DEFS:-} \
    -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen -I"$OUT" \
    "-DX0X_HOST_SOURCE=\"$SRC/host/x0x_host.c\"" \
    "-DSMC_MIXER_SOURCE=\"${SMC_SOURCE_FILE:-$SRC/firmware/src/app/smc_mixer.c}\"" \
    "$TEST_ROOT/tests/host/smc_drum_rs_test.c" $ORIGINAL_FLAGS $U "$OUT/engine.o" -lm -o "$OUT/probe"
"$OUT/probe"
