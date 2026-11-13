#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Focused host test for the PERF core-1 meter path.
set -eu
TEST_ROOT=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
SRC=${X0X_SOURCE_ROOT:-$TEST_ROOT}
OUT=${X0X_PERF_METER_OUT:-$(mktemp -d)}
cleanup() { [ -n "${X0X_PERF_METER_KEEP:-}" ] || rm -rf "$OUT"; }
trap 'cleanup' EXIT HUP INT TERM
cd "$SRC"
if [ ! -f build/gen/x0x_drum_samples.h ] || [ ! -f build/gen/felucca_font.h ]; then
    sh host/build_host.sh --generate-only
fi
# Miami on core 1 (JB_A = bit 2). Device images do not set X0X_CORE_PLAN.
WRAPPER="$OUT/cc-wrap"
cat > "$WRAPPER" <<'EOF'
#!/bin/sh
set -eu
CC="${IDLE_PROBE_CC:-cc}"
args=""
for a in "$@"; do
    if [ "$a" = "host/x0x_host.c" ]; then
        args="$args $IDLE_PROBE_ENTRY"
    else
        args="$args $a"
    fi
done
# shellcheck disable=SC2086
eval "exec $CC $args"
EOF
chmod +x "$WRAPPER"
export IDLE_PROBE_CC="${CC:-cc}"
export IDLE_PROBE_ENTRY="$SRC/tests/host/perf_meter_c1_test.c"
export CC="$WRAPPER"
export X0X_HOST_OUT="$OUT/perf_meter_c1"
export X0X_DEV_WOBBLE=0
export X0X_HOST_DEFS="-DX0X_CORE_PLAN=4 -D_POSIX_C_SOURCE=200809L"
sh host/build_host.sh
"$OUT/perf_meter_c1"
