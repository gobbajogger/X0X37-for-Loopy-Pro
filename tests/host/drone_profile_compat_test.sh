#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/../.."
OUT=${1:-build/host/drone-profile-compat}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
. tests/host/original_build.sh
SOURCES=$(python3 -c "import tools.build as b; print(' '.join(b.ORIGINAL_SYNTH_UNITS+b.ORIGINAL_SYNTH_BACKENDS))")
for profile in shipping historical; do
    profile_flags=
    [ "$profile" != historical ] || profile_flags=-DX0X_DRONE_RUNTIME_ENABLED=1
    for mode in normal sanitized; do
        flags=-O2
        [ "$mode" != sanitized ] || flags='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'
        exe="$OUT/$profile-$mode"
        ${CC:-cc} -std=c99 $flags -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
            -DX0X_HOST $ORIGINAL_FLAGS $profile_flags -Ifirmware/src -Ifirmware/src/app \
            -Ifirmware/src/dsp -Ibuild/gen tests/host/drone_profile_compat_test.c \
            firmware/src/app/preset_record.c $SOURCES -Wl,--gc-sections -lm \
            -o "$exe" > "$exe.compile.log" 2>&1
        "$exe" > "$exe.log" 2>&1
        cat "$exe.log"
    done
    cmp "$OUT/$profile-normal.log" "$OUT/$profile-sanitized.log"
done
# Inspect compiler dependencies, not optimised symbol names: the shipping
# runtime must not parse any Drone DSP definitions in the first place.
${CC:-cc} -std=c99 -DX0X_HOST $ORIGINAL_FLAGS -MM firmware/src/app/original_runtime.c > "$OUT/shipping-runtime.d"
if rg 'drone_string[.]h' "$OUT/shipping-runtime.d"; then
    echo 'FAIL: shipping runtime includes Drone DSP' >&2
    exit 1
fi
${CC:-cc} -std=c99 -DX0X_HOST $ORIGINAL_FLAGS -DX0X_DRONE_RUNTIME_ENABLED=1 -MM \
    firmware/src/app/original_runtime.c > "$OUT/historical-runtime.d"
rg -q 'drone_string[.]h' "$OUT/historical-runtime.d"
printf '%s\n' 'Drone DSP header absent in shipping dependency closure; present with historical opt-in: PASS'
