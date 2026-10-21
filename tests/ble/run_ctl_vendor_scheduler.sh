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

${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror -I"$HERE/../../firmware/hal" "$HERE/ctl_vendor_profile_test.c" -o "$OUT/profile"
"$OUT/profile"

${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror "$HERE/ctl_highcpu_test.c" -o "$OUT/highcpu"
"$OUT/highcpu"

${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror -no-pie "$HERE/ctl_host_scope_test.c" -o "$OUT/scope"
"$OUT/scope"

${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror -I"$HERE/../../firmware/hal" "$HERE/ctl_vendor_feedback_session_test.c" -o "$OUT/feedback"
for mode in queued pending drained backlog late duplicate pressure up-pressure controller-reset exhaustion-pressure; do
    "$OUT/feedback" "$mode"
done

${CC:-cc} -std=c11 -O1 -g -Wall -Wextra -Werror -I"$HERE/../../firmware/hal" "$HERE/ctl_vendor_bridge_test.c" -o "$OUT/bridge"
"$OUT/bridge"
python3 "$HERE/check_feedback_app.py"
python3 "$HERE/check_feedback_producer.py"
