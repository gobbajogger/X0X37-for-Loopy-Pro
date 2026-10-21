#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# tools/gate.sh expected_tests: a comment at the end of a run_tests.sh line must not add a test name.
# Example: "run x sh a.sh   # a test run never writes" must give only "x", not "never".
set -u
GATE=${X0X_GATE_SH:-tools/gate.sh}
T=$(mktemp -d "${TMPDIR:-/tmp}/gexp.XXXXXX"); trap 'rm -rf "$T"' EXIT
awk '/^expected_tests\(\) \{/,/^}/' "$GATE" > "$T/et.sh"
[ -s "$T/et.sh" ] || { echo "FAIL: no expected_tests in $GATE"; exit 1; }
mkdir -p "$T/w/tests/host" "$T/w/tests/scenarios" "$T/w/build"
cat > "$T/w/tests/run_tests.sh" <<'R'
# run comment_only sh x.sh
run alpha sh a.sh   # a test run never writes the live folder
gen_wait() {   # 0 if the test must wait (host-build runs in this run and did not end)
    true
}
run "beta" sh b.sh
R
printf '  ok   alpha\n  ok   beta\n' > "$T/tests.log"
( cd "$T/w" && . "$T/et.sh" && expected_tests "$T/tests.log" "$T/exp.txt" "$T/miss.txt" )
got=$(tr '\n' ' ' < "$T/exp.txt"); miss=$(cat "$T/miss.txt")
fail=0
[ "$got" = "alpha beta " ] || { echo "FAIL: expected names [$got], want [alpha beta ]"; fail=1; }
[ -z "$miss" ] || { echo "FAIL: missing [$miss], want none"; fail=1; }
[ $fail = 0 ] && echo "gate-expected: 2 of 2 ok"
exit $fail
