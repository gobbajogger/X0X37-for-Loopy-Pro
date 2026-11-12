#!/bin/sh
# Build the production UI in the host simulator and check feedback guards.
set -eu
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
. tests/host/original_build.sh
U=$ORIGINAL_SOURCES
HOST_DEFS=
if [ "$(uname -s)" = Darwin ]; then HOST_DEFS=-D_DARWIN_C_SOURCE; fi
X0X_HOST_OUT="$OUT/host" X0X_HOST_DEFS="$HOST_DEFS" sh host/build_host.sh --generate-only > "$OUT/build.log" 2>&1 || {
    cat "$OUT/build.log"
    exit 1
}
# shellcheck disable=SC2086
${CC:-cc} -O1 -g -ffp-contract=off -std=c99 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
    -DX0X_HOST $HOST_DEFS -Ifirmware/src -Ifirmware/src/dsp -Ibuild/gen \
    tests/host/ui_feedback_test.c $ORIGINAL_FLAGS $U -lm -o "$OUT/test"
"$OUT/test" "$@"
