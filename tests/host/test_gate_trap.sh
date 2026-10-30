#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# tools/gate.sh and tools/gate_sigwrap.py: checks 5 and 13 of rules v4.
#   5 (rule 2, no pkill -f): a stop signal names its sender. The KILLED lines have "from pid N" and the cwd of the sender.
#  13 (rule 3): a gate script that changes during the run gives "SCRIPT CHANGED" in gate.log.
# Cases:
#   1. gate_sigwrap.py with the fixture child inner.sh: a sender in a known folder sends SIGTERM -> the child sees
#      "sig=SIGTERM pid=<sender> cwd=<folder>", the wrapper exits 143 and removes the sender file.
#   2. A real gate run on a temp repository (fake toolchain and SDK files made at run time, the fixture build.sh waits):
#      the test changes gate_flashmap.py beside the gate copy, then a sender sends SIGTERM to the gate PID.
#      gate.log: "KILLED by SIGTERM from pid <sender> (cwd <folder>" and "SCRIPT CHANGED". Output: "KILLED <commit> SIGTERM from pid".
#   3. The same with X0X_GATE_INNER=1 (no wrapper): "sender unknown".
# The work folder is under build/ of this tree (the gate moves a /tmp work folder to the live cache, so not /tmp).
set -u
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; FX="$TOP/tests/host/fixtures/gate_trap"
mkdir -p "$TOP/build"
T=$(mktemp -d "$TOP/build/gatetrap.XXXXXX"); trap 'pkill -P $$ 2>/dev/null; rm -rf "$T"' EXIT
fails=0; n=0
ok()  { n=$((n + 1)); echo "ok   $1"; }
bad() { n=$((n + 1)); fails=$((fails + 1)); echo "FAIL $1"; }
waitfor() { local i=0; while [ $i -lt 100 ]; do grep -q "$2" "$1" 2>/dev/null && return 0; sleep 0.1; i=$((i + 1)); done; return 1; }
# a sender: a subshell in folder $2 sends SIGTERM to $1 and stays alive 3 s (so its cwd is readable); prints its PID
send() { mkdir -p "$2"; ( cd "$2" && kill -TERM "$1" && sleep 3 ) & echo $!; }

# 1. the wrapper alone
SF="$T/sig1"; : > "$SF"
X0X_GATE_SIGFILE="$SF" python3 "$TOP/tools/gate_sigwrap.py" "$SF" sh "$FX/inner.sh" "$T/ready1" > "$T/w1.out" 2>&1 & WP=$!
waitfor "$T/ready1" ready || bad "wrapper: the child did not start"
SP=$(send "$WP" "$T/sender1"); wait "$WP"; rc=$?
if [ $rc = 143 ] && grep -q "INNER GOT sig=SIGTERM pid=$SP cwd=$T/sender1 " "$T/w1.out" && [ ! -e "$SF" ]; then
    ok "wrapper: the child gets the sender (pid $SP, cwd sender1), exit 143, sender file removed"
else bad "wrapper: rc=$rc sender=$SP out=$(cat "$T/w1.out") file=$(ls "$SF" 2>&1)"; fi

# 2. and 3. a gate run on a temp repository
G="$T/g"; mkdir -p "$G"; cp "$TOP/tools/gate.sh" "$TOP/tools/gate_sigwrap.py" "$TOP/tools/gate_flashmap.py" "$TOP/tools/validation_inputs.py" "$G/"
mkdir -p "$T/tc/pi32v2/bin" "$T/sdk/cpu/wl82/tools" "$T/res" "$T/refs"
printf '#!/bin/sh\nexit 0\n' > "$T/tc/pi32v2/bin/clang"; chmod +x "$T/tc/pi32v2/bin/clang"; : > "$T/sdk/cpu/wl82/tools/uboot.boot"
printf '#!/bin/sh\nexit 0\n' > "$T/pre-push"
git init -q --bare "$T/box.git"; git init -q -b integration "$T/c"; cp "$FX/build.sh" "$T/c/build.sh"
git -C "$T/c" add . && git -C "$T/c" -c user.name=t -c user.email=t@t commit -qm "Fixture build" && git -C "$T/c" push -q "$T/box.git" integration
H=$(git -C "$T/c" rev-parse --short=7 HEAD)
git clone -q --no-local "$T/c" "$T/refs/fixture"
PIN="fixture=$(git -C "$T/c" rev-parse HEAD)"
gate() {  # gate <case> [env...]: start the gate in the background; GP = its PID
    local c=$1; shift
    env X0X_REPO="$T/box.git" X0X_RESULTS="$T/res" X0X_REFS="$T/refs" X0X_GATE_WORK="$T/work$c" X0X_TOOLCHAIN="$T/tc" \
        AC79_SDK="$T/sdk" X0X_PREPUSH_HOOK="$T/pre-push" X0X_GATE_LOCK="$T/gate.lock" X0X_REF_PINS="$PIN" X0X_GATE_SIGFILE= "$@" \
        bash "$G/gate.sh" "$H" --purpose owner-validation --reason "Gate signal regression fixture requested by rule repair" > "$T/gate$c.out" 2>&1 & GP=$!
}
waitbuild() {
    local i=0
    while [ $i -lt 200 ]; do
        find "$1" -name build-elite.log -exec grep -q 'fixture build' {} \; -print 2>/dev/null | grep -q . && return 0
        sleep 0.1; i=$((i + 1))
    done
    return 1
}
gate 2
if waitbuild "$T/work2"; then
    sleep 1.2; touch "$G/gate_flashmap.py"
    SP=$(send "$GP" "$T/sender2"); wait "$GP"; rc=$?; L2=$(find "$T/work2" -name gate.log)
    if grep -q "KILLED by SIGTERM from pid $SP (cwd $T/sender2;" "$L2"; then ok "gate.log names the sender (pid $SP, cwd sender2)"; else bad "gate.log: $(grep KILLED "$L2")"; fi
    if grep -q "^KILLED $H SIGTERM from pid $SP cwd $T/sender2 (no result)" "$T/gate2.out" && [ $rc = 143 ]; then ok "output line names the sender, exit 143"; else bad "output: rc=$rc $(cat "$T/gate2.out")"; fi
    if grep -q "SCRIPT CHANGED during the run: $G/gate_flashmap.py" "$L2"; then ok "gate.log: SCRIPT CHANGED for gate_flashmap.py"; else bad "no SCRIPT CHANGED line: $(cat "$L2")"; fi
else bad "gate 2 did not reach the builds: $(cat "$T/gate2.out")"; kill "$GP" 2>/dev/null; fi
gate 3 X0X_GATE_INNER=1
if waitbuild "$T/work3"; then
    SP=$(send "$GP" "$T/sender3"); wait "$GP"; rc=$?; L3=$(find "$T/work3" -name gate.log)
    if grep -q "KILLED by SIGTERM sender unknown (no result)" "$L3" && [ $rc = 143 ] && ! grep -q 'SCRIPT CHANGED' "$L3"; then ok "no wrapper: sender unknown, no SCRIPT CHANGED"; else bad "no wrapper: rc=$rc $(grep -E 'KILLED|SCRIPT' "$L3")"; fi
else bad "gate 3 did not reach the builds: $(cat "$T/gate3.out")"; kill "$GP" 2>/dev/null; fi
echo "gate trap: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
