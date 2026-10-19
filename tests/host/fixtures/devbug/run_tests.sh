#!/bin/sh
# A small tests/run_tests.sh for the device bug test of tools/quickcheck.sh (same result lines, X0X_TESTS filter).
cd "$(dirname "$0")/.." || exit 1
X0X_TESTS="${X0X_TESTS:-}"; FAIL=0
run() { name=$1; shift; if [ -n "$X0X_TESTS" ]; then case " $X0X_TESTS " in *" $name "*) ;; *) return ;; esac; fi
        if "$@" >/dev/null 2>&1; then echo "  ok   $name"; else echo "  FAIL $name"; FAIL=1; fi; }
run app sh -c true
[ -f tests/pad_inlog_test.sh ] && run pad-inlog sh tests/pad_inlog_test.sh
[ -f tests/weak_inlog_test.sh ] && run weak-inlog sh tests/weak_inlog_test.sh
[ -f tests/replay_test.sh ] && run replay sh tests/replay_test.sh
exit $FAIL
