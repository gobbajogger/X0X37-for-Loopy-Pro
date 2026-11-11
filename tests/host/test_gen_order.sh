#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tests/run_tests.sh: a test that includes build/gen (drum-pan, stereo) starts only after host-build ends.
# A fresh copy of the tree (no build/) runs drum-pan, stereo and host-build with 8 jobs, then with 1 job.
# A cc wrapper writes the state of build/gen and build/host/x0x_host when a drum-pan or stereo compile starts.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
D="$(cd "$(dirname "$0")/../.." && pwd)"
T=$(mktemp -d "${TMPDIR:-/tmp}/gord.XXXXXX"); trap 'rm -rf "$T"' EXIT
REAL_CC="${CC:-cc}"
cat > "$T/cc" <<W
#!/bin/sh
for a in "\$@"; do
    case "\$a" in
    tests/host/drum_pan_test.c|tests/host/stereo_test.c)
        g=no; [ -f build/gen/x0x_drum_tables.h ] && g=yes
        h=no; [ -f build/host/x0x_host ] && h=yes
        echo "\$(basename "\$a" .c) gen=\$g host=\$h" >> "$T/cc.log" ;;
    esac
done
exec $REAL_CC "\$@"
W
chmod 755 "$T/cc"
fails=0
for j in 8 1; do
    rm -rf "$T/tree" "$T/cc.log"; mkdir -p "$T/tree"
    (cd "$D" && tar cf - --exclude=./build --exclude=./.git .) | (cd "$T/tree" && tar xf -) || { echo "FAIL copy of the tree"; exit 1; }
    out=$(cd "$T/tree" && CC="$T/cc" X0X_TEST_JOBS=$j X0X_TESTS="drum-pan stereo host-build" sh tests/run_tests.sh 2>&1); rc=$?
    for t in drum_pan_test stereo_test; do
        l=$(grep "^$t " "$T/cc.log" 2>/dev/null | head -1)
        if [ "$l" = "$t gen=yes host=yes" ]; then echo "ok   jobs=$j: $l"
        else echo "FAIL jobs=$j: want \"$t gen=yes host=yes\", got \"$l\""; fails=$((fails + 1)); fi
    done
    if [ $rc = 0 ]; then echo "ok   jobs=$j: run_tests.sh passed"
    else echo "FAIL jobs=$j: run_tests.sh rc=$rc:"; echo "$out" | sed 's/^/       /'; fails=$((fails + 1)); fi
done
[ $fails = 0 ] && echo "gen order: all passed" || { echo "gen order: $fails FAILED"; exit 1; }
