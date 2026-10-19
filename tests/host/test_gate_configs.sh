#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# tools/gate.sh builds and tests two configs (dev X0X_DEV_PADCAL=1, rel 0) and removes the build variables of the
# caller. Static checks of the script (the full proof is a scratch gate run). tools/require_gate_pass.sh reads the
# rel lines that the gate writes (rel/build/...).
set -u
D="$(cd "$(dirname "$0")/../.." && pwd)"; G="$D/tools/gate.sh"; Q="$D/tools/require_gate_pass.sh"
fails=0; n=0
chk() { n=$((n + 1)); if grep -qF -- "$2" "$3"; then echo "ok   $1"; else echo "FAIL $1: no '$2' in $(basename "$3")"; fails=$((fails + 1)); fi; }
bash -n "$G" || { echo "FAIL gate.sh syntax"; exit 1; }
chk "the gate removes X0X_DEV_* of the caller" "compgen -e | grep -E '^X0X_DEV_'" "$G"
BV=$(sed -n 's/^BUILD_VARS="\(.*\)"$/ \1 /p' "$G")
for v in X0X_CDC X0X_DEBUG FELUCCA_DATA_FAMILY MODEL BOARD; do
    n=$((n + 1)); case "$BV" in *" $v "*) echo "ok   the gate removes $v of the caller" ;; *) echo "FAIL BUILD_VARS has no $v"; fails=$((fails + 1)) ;; esac
done
chk "dev build with X0X_DEV_PADCAL=1" 'build_config "$1" "$2" "$3" 1 ""' "$G"
chk "rel build with X0X_DEV_PADCAL=0 in its own worktree" 'build_config "$4" "$5" "$6" 0 -rel' "$G"
chk "dev tests with X0X_DEV_PADCAL=1" 'X0X_DEV_PADCAL=1 sh tests/run_tests.sh > "$L/tests.log"' "$G"
chk "rel tests with X0X_DEV_PADCAL=0" 'X0X_DEV_PADCAL=0 sh tests/run_tests.sh > "$L/tests-rel.log"' "$G"
chk "rel test failures make a FAIL" 'check_tests "$WR" "$L/tests-rel.log" "$TRCR" rel -rel' "$G"
chk "pool headroom checked for the rel builds" 'for b in elite pro fm1 elite-rel pro-rel fm1-rel' "$G"
chk "rel lines in the CRC manifest" 'builds("rel/")' "$G"
chk "Config CRCs line in the result file" 'echo "- Config CRCs: $CFG"' "$G"
chk "Configs column in the index row" '| $NOW | $CFG |" >> "$INDEX"' "$G"
chk "guard: rel lines" 'PRE="rel/build/"' "$Q"
chk "guard: one-config result is dev only" '^- Config CRCs: ' "$Q"
# the index row stays parseable: columns 2-7 are the same as before (commit, tree, git tree, result, file, time)
n=$((n + 1)); row='| be9a097 | bd2688f0437c | f521eba896d4 | PASS | x-1.md (also x.md) | 2026-10-17 00:27 +10 | dev elite 38d57833, ble e63a25b3; rel elite 8ea0ba13, ble 504fe793 |'
got=$(echo "$row" | awk -F '|' '{ r=$5; f=$6; t=$7; gsub(/^ +| +$/, "", r); gsub(/^ +| +$/, "", f); gsub(/^ +| +$/, "", t); split(f, a, " "); print r ";" a[1] ";" t }')
[ "$got" = "PASS;x-1.md;2026-10-17 00:27 +10" ] && echo "ok   index row columns 5-7: $got" || { echo "FAIL index row columns: $got"; fails=$((fails + 1)); }
# Execute the gate's parser against comments, quoted hashes, and a missing real test.
n=$((n + 1))
if python3 - "$G" <<'PYTEST'
import pathlib, subprocess, sys, tempfile
source = pathlib.Path(sys.argv[1]).read_text()
parser = source.split('expected_tests() {', 1)[1].split("<<'PY' > \"$3\"\n", 1)[1].split('\nPY\n', 1)[0]
with tempfile.TemporaryDirectory() as folder:
    root = pathlib.Path(folder)
    (root / 'tests').mkdir()
    (root / 'tests/run_tests.sh').write_text(
        'export X0X_TEST_RUN=1 # a test run never writes live state\n'
        'gen_wait() { # this run and did not end\n'
        'run present echo "# quoted hash"; run missing true # run bogus true\n')
    (root / 'tests.log').write_text('  ok   present\n')
    result = subprocess.run([sys.executable, '-c', parser, 'tests.log', 'expected'],
                            cwd=root, capture_output=True, text=True, check=True)
    assert (root / 'expected').read_text().splitlines() == ['missing', 'present']
    assert result.stdout.strip() == 'missing', result.stdout
print('ok   expected tests ignore comments and retain missing real tests')
PYTEST
then :; else fails=$((fails + 1)); echo "FAIL expected tests parser"; fi
echo "gate configs: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
