#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# tools/status.sh, checks 10, 11, 13, 15 and 18 of rules v4, on a temp bare repository and temp folders.
#   10: the hook log has no "QUICK PASS" line and the skip log no skip line for the integration tip -> INTEGRATION NO QUICK PASS.
#   11: 2 gate runs of one commit in the last 24 h, the later one without "(force: ...)" -> GATE TWICE. A force run, a
#       "result used again" row and rows older than 24 h do not count.
#   13: the status.sh mtime changed during the run (X0X_STATUS_MTIME0) -> STATUS SCRIPT CHANGED; a gate.log with
#       "gate: SCRIPT CHANGED" -> SCRIPT CHANGED.
#   15: an agent *.md file of the last 60 min with "should work" (not in double quotes) -> STOP WORDS.
#   18: the last two rows with a result in BLE_ATTEMPTS.md are FAIL -> BISECT NEEDED.
#   10 end to end: the update hook (tools/update.hook, installed in a temp bare repository) writes a "QUICK PASS" line
#       and a "QUICK FAIL" line to x0x37.hookLog; status.sh accepts the tip after the QUICK PASS push and flags it after
#       a git update-ref (no hook).
# Fixtures: tests/host/fixtures/status2/.
set -u
export X0X_TEST_RUN=1   # check 30: status.sh refuses the live folder
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; S="$TOP/tools/status.sh"; F="$TOP/tests/host/fixtures/status2"
T=$(mktemp -d "${TMPDIR:-/tmp}/statusr2.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
C="$T/c"; git init -q -b integration "$C" && git -C "$C" config user.name t && git -C "$C" config user.email t@t || { echo "FAIL setup"; exit 1; }
mkdir -p "$T/res" "$T/agents/a" "$T/gl/x.logs" "$T/gl0"
sed "s/NOW/$(date '+%Y-%m-%d %H:%M %z' | sed 's/00$//')/" "$F/index.md" > "$T/res/index.md"
run() {  # run <BLE_ATTEMPTS fixture> <hook log line or -> [ENV=VALUE ...]: the status.md text
    local ble=$1 hl=$2; shift 2
    rm -rf "$T/box.git" "$T/out" "$T/tmp"; git init -q --bare "$T/box.git"; mkdir -p "$T/out" "$T/tmp"; : > "$T/gate.lock"
    cp "$F/$ble" "$C/BLE_ATTEMPTS.md" && git -C "$C" add . && git -C "$C" commit -q --allow-empty -m "Attempts $ble" && git -C "$C" push -q -f "$T/box.git" integration 2>/dev/null
    H=$(git -C "$C" rev-parse HEAD)
    case " $* " in *" SKIPNOW=1 "*) echo "2026-10-17 03:00:00 +1000 server-update refs/heads/integration 1234567..${H:0:7} skip: owner OK 03:00" > "$T/res/skips.log";; esac
    : > "$T/res/hook.log"; [ "$hl" = - ] || echo "2026-10-17 03:00:00 +1000 refs/heads/integration 1234567..${H:0:7} $hl" > "$T/res/hook.log"
    env X0X_REPO="$T/box.git" X0X_STATUS_OUT="$T/out" X0X_STATUS_TMP="$T/tmp" X0X_RESULTS_DIR="$T/res" X0X_GATE_LOCK="$T/gate.lock" \
        X0X_HOOK_LOG="$T/res/hook.log" X0X_SKIP_LOG="$T/res/skips.log" X0X_AGENT_DIRS="$T/agents" X0X_GATE_LOG_ROOTS="$T/gl0" "$@" bash "$S" >/dev/null 2>&1
    cat "$T/out/status.md" 2>/dev/null
}
chk() {  # chk <name> <yes|no> <fixed text> <status text>
    n=$((n + 1)); if printf '%s\n' "$4" | grep -qF -- "$3"; then got=yes; else got=no; fi
    if [ "$got" = "$2" ]; then echo "ok   $1"; else echo "FAIL $1: want $2 \"$3\""; printf '%s\n' "$4" | grep -E 'check 1[0-8]' | sed 's/^/     /'; fails=$((fails + 1)); fi
}
cp "$F/agent/notes.md" "$F/agent/clean.md" "$T/agents/a/"; cp "$F/gate.log" "$T/gl/x.logs/gate.log"
BAD=$(run BLE_ATTEMPTS-2fail.md "QUICK FAIL abc 3.0 s" X0X_STATUS_MTIME0=1 X0X_GATE_LOG_ROOTS="$T/gl"); H=$(git -C "$C" rev-parse HEAD)
chk "10: no QUICK PASS line" yes "**INTEGRATION NO QUICK PASS** (check 10): ${H:0:7} has no QUICK PASS line" "$BAD"
chk "11: aaaaaaa twice" yes "**GATE TWICE** (check 11): aaaaaaa (2 runs)" "$BAD"
L11=$(printf '%s\n' "$BAD" | grep -F 'check 11')
chk "11: force run counts not" no "bbbbbbb" "$L11"
chk "11: result used again counts not" no "ccccccc" "$L11"
chk "11: rows older than 24 h count not" no "ddddddd" "$L11"
chk "13: status.sh changed" yes "**STATUS SCRIPT CHANGED** (check 13)" "$BAD"
chk "13: gate.log SCRIPT CHANGED line" yes "x: /workspace/x0x37-refs/gate/gate_flashmap.py 02:00:01 -> 02:00:09" "$BAD"
chk "15: should work" yes "**STOP WORDS** (check 15): $T/agents/a/notes.md:3: This fix should work" "$BAD"
L15=$(printf '%s\n' "$BAD" | grep -F 'check 15')
chk "15: a quoted word is not a hit" no "notes.md:2" "$L15"
chk "18: two FAIL rows" yes "**BISECT NEEDED** (check 18): attempts 3 and 4 FAIL in a row" "$BAD"
SC=$(run BLE_ATTEMPTS-2fail.md - X0X_GATE_LOG_ROOTS="$T/gl"); H=$(git -C "$C" rev-parse HEAD)
chk "13: gate.log only" yes "**SCRIPT CHANGED** (check 13): a gate script changed during a gate run: x: " "$SC"
SK=$(run BLE_ATTEMPTS-2fail.md - SKIPNOW=1); H=$(git -C "$C" rev-parse HEAD)
chk "10: a skip line is enough" yes "has a skip line in the skip log" "$SK"
rm -f "$T/res/skips.log" "$T/agents/a/notes.md"
GOOD=$(run BLE_ATTEMPTS-ok.md "QUICK PASS abc 3.0 s (20 tests ok)"); H=$(git -C "$C" rev-parse HEAD)
chk "10: QUICK PASS line" yes "- Integration quick check (check 10): ${H:0:7} has a QUICK PASS line in the hook log." "$GOOD"
chk "13: no change" yes "- Script change (check 13): none." "$GOOD"
chk "15: no stop word" yes "- Stop words (check 15): none in 1 agent *.md files changed in the last 60 min." "$GOOD"
chk "18: PASS at the end" yes "- BLE attempts (check 18): 3 rows with a result, no 2 FAIL rows at the end." "$GOOD"
touch -d '3 hours ago' "$T/agents/a/clean.md"; echo "It should work." > "$T/agents/a/old.md"; touch -d '2 hours ago' "$T/agents/a/old.md"
OLD=$(run BLE_ATTEMPTS-ok.md -); H=$(git -C "$C" rev-parse HEAD)
chk "15: files older than 60 min count not" yes "- Stop words (check 15): none in 0 agent" "$OLD"
# check 10 end to end, with the update hook
B="$T/hb.git"; git init -q --bare "$B"; bash "$TOP/tools/install_server_hooks.sh" "$B" >/dev/null || { echo "FAIL hooks"; exit 1; }
printf '#!/bin/bash\nste_step() { :; }\nif [ -f "$1/qcfail" ]; then echo "QUICK FAIL x 1.0 s"; exit 1; fi\necho "QUICK PASS x 1.0 s (1 tests ok)"\n' > "$T/qc.sh"; chmod +x "$T/qc.sh"
for k in "workDir $T/hwd" "skipLog $T/hskips.log" "quickcheck $T/qc.sh" "gateLock $T/gate.lock" "hookLog $T/hook2.log"; do set -- $k; git -C "$B" config "x0x37.$1" "$2"; done
hpush() { git -C "$C" push -q -o by=merge-integrator "$B" integration 2>/dev/null; }
git -C "$C" commit -q --allow-empty -m "Hook base" && hpush; H1=$(git -C "$C" rev-parse HEAD)
touch "$C/qcfail" && git -C "$C" add qcfail && git -C "$C" commit -qm "Bad" && ! hpush; HB=$(git -C "$C" rev-parse HEAD)
n=$((n + 1)); if grep -q "refs/heads/integration 0000000..${H1:0:7} QUICK PASS x" "$T/hook2.log" && grep -q "\.\.${HB:0:7} QUICK FAIL x" "$T/hook2.log"; then echo "ok   10: the hook writes QUICK PASS and QUICK FAIL lines"
else echo "FAIL 10: hook log lines"; sed 's/^/     /' "$T/hook2.log"; fails=$((fails + 1)); fi
st2() { rm -rf "$T/out" "$T/tmp"; mkdir -p "$T/out" "$T/tmp"; X0X_REPO="$B" X0X_STATUS_OUT="$T/out" X0X_STATUS_TMP="$T/tmp" X0X_RESULTS_DIR="$T/res" \
    X0X_GATE_LOCK="$T/gate.lock" X0X_HOOK_LOG="$T/hook2.log" X0X_SKIP_LOG="$T/hskips.log" X0X_AGENT_DIRS="$T/agents" X0X_GATE_LOG_ROOTS="$T/gl0" \
    bash "$S" >/dev/null 2>&1; cat "$T/out/status.md"; }
chk "10: tip after a QUICK PASS push" yes "${H1:0:7} has a QUICK PASS line in the hook log" "$(st2)"
git -C "$B" update-ref refs/heads/integration "$HB"
chk "10: tip after git update-ref" yes "**INTEGRATION NO QUICK PASS** (check 10): ${HB:0:7}" "$(st2)"
echo "status rules 2: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
