#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Native application callbacks only; no vendor execution, radio, or HCI replay.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
FLAGS=
case ${1:-} in
    '') ;;
    --sanitized) FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' ;;
    *) echo "usage: $0 [--sanitized]" >&2; exit 2 ;;
esac
for TEST in ctl_vendor_profile ctl_vendor_subscription ctl_vendor_security_retry ctl_vendor_security_backpressure ctl_vendor_security_edge ctl_vendor_security_mailbox ctl_vendor_uuid; do
    ${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror $FLAGS -DCHECK_INTERNAL_BUDGET=1 -I"$HERE/../../firmware/hal" \
        "$HERE/${TEST}_test.c" -o "$OUT/$TEST"
    # LeakSanitizer cannot operate under this executor's ptrace. No test heap use.
    ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$OUT/$TEST"
done
