#!/bin/sh
# Production UI/engine tests with ordinary and optional ASan/UBSan host builds.
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/host/project-runtime}
mkdir -p "$OUT"
awk '/Project transfers cannot queue an update/{on=1}
     on && /^#if X0X_CDC/{exit} on{print}' firmware/src/app/main_fm1.c > "$OUT/project_updates.inc"
test -s "$OUT/project_updates.inc"
${CC:-cc} -std=c99 -Wall -Wextra -Werror -I"$OUT" \
    tests/host/project_update_guard_test.c -o "$OUT/project_update_guard"
"$OUT/project_update_guard"
sh host/build_host.sh --generate-only
U="firmware/src/dsp/drum909.c firmware/src/dsp/drum808.c firmware/src/dsp/x0x_param.c firmware/src/dsp/breaks.c
firmware/src/dsp/fxbus.c firmware/src/dsp/master.c firmware/src/seq/sequencer.c firmware/src/seq/tb3po.c
firmware/src/seq/pattern.c firmware/src/seq/motion.c firmware/src/app/engine.c"
for test in project_runtime project_smc_runtime; do
    # shellcheck disable=SC2086
    ${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
        -DX0X_HOST -D_POSIX_C_SOURCE=200809L ${X0X_HOST_DEFS:-} ${SAN_FLAGS:-} \
        -Ifirmware/src -Ifirmware/src/app -Ifirmware/src/dsp -Ibuild/gen \
        "tests/host/${test}_test.c" $U -lm -o "$OUT/$test"
    "$OUT/$test"
done
