#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# When Pillow is missing, run_tests.sh calls gen_nonfont instead of host-build.
# Samples can exist without x0x_drum_tables.h; a custom X0X_BREAK_BANK skips
# gen_builtin_break.sh, so gen_nonfont must regenerate drums when tables vanish
# (same gate as host/build_host.sh). Otherwise stereo / drum-pan fail to compile.
#
# Nested run_tests shares parent build/host/iso. Clearing .gen-done for the
# probe must not strand parallel needs_gen waiters in the parent suite — always
# republish .gen-done (tables present) or .gen-fail before exit.
set -eu
cd "$(dirname "$0")/../.."
T=$(mktemp -d "${TMPDIR:-/tmp}/x0x-gen-nonfont.XXXXXX")
publish_parent_gen() {
    mkdir -p build/host/iso
    if [ -f build/gen/x0x_drum_tables.h ]; then
        : > build/host/iso/.gen-done
        rm -f build/host/iso/.gen-fail
    else
        : > build/host/iso/.gen-fail
    fi
}
trap 'publish_parent_gen; rm -rf "$T"' EXIT HUP INT TERM
mkdir -p "$T/nopil" "$T/bank" build/gen build/host
: > "$T/bank/bank.txt"
printf 'stub\n' > build/gen/x0x_drum_samples.h
rm -f build/gen/x0x_drum_tables.h build/gen/x0x_break_bank.h build/host/iso/.gen-done build/host/iso/.gen-fail

# Hide user-site Pillow so host-build is not-run and gen_nonfont runs.
out=$(PYTHONUSERBASE="$T/nopil" X0X_BREAK_BANK="$T/bank" X0X_TEST_JOBS=1 \
    X0X_TESTS='host-build stereo' sh tests/run_tests.sh 2>&1) || {
    printf '%s\n' "$out" | sed 's/^/       /' >&2
    echo "FAIL: host-build/stereo under samples-only + X0X_BREAK_BANK" >&2
    exit 1
}
printf '%s\n' "$out" | grep -q 'not-run host-build (Pillow)' || {
    echo "FAIL: expected host-build Pillow not-run" >&2
    printf '%s\n' "$out" | sed 's/^/       /' >&2
    exit 1
}
printf '%s\n' "$out" | grep -q 'ok   stereo' || {
    echo "FAIL: expected stereo ok" >&2
    printf '%s\n' "$out" | sed 's/^/       /' >&2
    exit 1
}
[ -f build/gen/x0x_drum_tables.h ] || {
    echo "FAIL: gen_nonfont left x0x_drum_tables.h missing" >&2
    exit 1
}
publish_parent_gen
[ -e build/host/iso/.gen-done ] || {
    echo "FAIL: parent needs_gen marker .gen-done missing after probe" >&2
    exit 1
}
echo "gen-nonfont-tables: ok"
