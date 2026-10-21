#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# Native control-flow tests only; target stack sizing and radio are not exercised.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror "$HERE/ctl_vendor_scheduler_test.c" -o "$OUT/test"
"$OUT/test"

${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror "$HERE/ctl_vendor_events_test.c" -o "$OUT/events"
"$OUT/events"

${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror "$HERE/ctl_vendor_profile_test.c" -o "$OUT/profile"
"$OUT/profile"
