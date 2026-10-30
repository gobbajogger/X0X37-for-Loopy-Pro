#!/bin/bash
# tools/status.sh, check 30 of rules v4: test isolation. A test never writes the live orchestrator folder.
# Cases:
#   1. X0X_TEST_RUN=1 and no output folder: status.sh refuses (exit 4) and writes nothing in the live folder
#      (here X0X_STATUS_LIVEDIR = a temp folder, so the case never touches the real live folder).
#   2. "--out DIR" and 3. X0X_STATUS_OUT=DIR: status.md and LIVE_BRANCHES go to DIR, the temp files to DIR/.status-tmp,
#      and the live folder stays empty. 4. an unknown argument: exit 2.
#   5. Scan: each tests/host/*.sh that runs tools/status.sh gives it an output folder (X0X_STATUS_OUT or --out).
#      The bad fixture fixtures/status/leaky_status_test.sh must be flagged.
#   6. The real live files /workspace/x0x37-orchestrator/status.md and LIVE_BRANCHES (X0X_ISO_LIVE sets the folder)
#      have the same sha256 and mtime before and after a run of the status tests. Cron writes them every 5 minutes,
#      so a change is checked again one time. A leak repeats, a cron write does not.
set -u
export X0X_TEST_RUN=1
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; S="$TOP/tools/status.sh"; F="$TOP/tests/host/fixtures/status"
T=$(mktemp -d "${TMPDIR:-/tmp}/statusiso.XXXXXX"); trap 'rm -rf "$T"' EXIT
n=0; fails=0
ok()  { n=$((n + 1)); echo "ok   $1"; }
bad() { n=$((n + 1)); fails=$((fails + 1)); echo "FAIL $1"; }
B="$T/box.git"; git init -q --bare "$B"; C="$T/c"; git init -q -b integration "$C"
git -C "$C" -c user.name=t -c user.email=t@t commit -q --allow-empty -m "Base"; git -C "$C" push -q "$B" integration
mkdir -p "$T/live" "$T/res"
ENVV=(X0X_REPO="$B" X0X_STATUS_LIVEDIR="$T/live" X0X_RESULTS_DIR="$T/res" X0X_GATE_LOCK="$T/gate.lock" X0X_PREFS_LINT=/bin/true)
empty_live() { [ -z "$(ls -A "$T/live")" ]; }

# 1
out=$(env "${ENVV[@]}" bash "$S" 2>&1); rc=$?
if [ $rc = 4 ] && empty_live && echo "$out" | grep -q 'REFUSED'; then ok "test run, no output folder: refused, exit 4"; else bad "test run, no output folder: rc=$rc live=$(ls -A "$T/live"|tr '\n' ' ') $out"; fi
# 2
out=$(env "${ENVV[@]}" bash "$S" --out "$T/o1" 2>&1); rc=$?
if [ $rc = 0 ] && [ -s "$T/o1/status.md" ] && [ -f "$T/o1/LIVE_BRANCHES" ] && [ -d "$T/o1/.status-tmp" ] && empty_live; then ok "--out DIR: files in DIR only"; else bad "--out DIR: rc=$rc $(ls -A "$T/o1" 2>&1|tr '\n' ' ') live=$(ls -A "$T/live"|tr '\n' ' ') $out"; fi
# 3
out=$(env "${ENVV[@]}" X0X_STATUS_OUT="$T/o2" bash "$S" 2>&1); rc=$?
if [ $rc = 0 ] && [ -s "$T/o2/status.md" ] && [ -f "$T/o2/LIVE_BRANCHES" ] && empty_live; then ok "X0X_STATUS_OUT: files in DIR only"; else bad "X0X_STATUS_OUT: rc=$rc live=$(ls -A "$T/live"|tr '\n' ' ') $out"; fi
# 4
out=$(env "${ENVV[@]}" bash "$S" --bogus 2>&1); rc=$?
if [ $rc = 2 ] && empty_live; then ok "unknown argument: exit 2"; else bad "unknown argument: rc=$rc $out"; fi

# 5 scan: a file that runs tools/status.sh must name X0X_STATUS_OUT or --out on that line
scan() { # prints the bad files. Joins "\" lines, drops comments, finds a run of status.sh (or of $S = status.sh).
  for f in "$@"; do
    awk -v f="$f" '
      { line = line $0 } /\\$/ { sub(/\\$/, "", line); next }
      { l = line; line = ""; sub(/(^|[[:space:]])#.*$/, "", l)
        if (l ~ /S="?[^ ]*tools\/status\.sh/) { hasS = 1; next }
        run = (l ~ /(bash|sh)[[:space:]]+"?[^ ]*tools\/status\.sh/) || (hasS && l ~ /(bash|sh)[[:space:]]+"?\$S"?([[:space:]]|$|\))/)
        if (run && l !~ /X0X_STATUS_OUT|--out/) { print f ":" NR ": " l; exit } }' "$f"
  done; }
r=$(scan "$F/leaky_status_test.sh")
if [ -n "$r" ]; then ok "scan flags the bad fixture: $r"; else bad "scan does not flag fixtures/status/leaky_status_test.sh"; fi
r=$(scan $(ls "$TOP"/tests/host/*.sh | grep -v '/test_status_isolation\.sh$'))   # this file: case 1 uses a temp live folder
if [ -z "$r" ]; then ok "scan: each status test gives an output folder"; else bad "scan: no output folder: $r"; fi

# 6 the real live files
LIVE=${X0X_ISO_LIVE:-/workspace/x0x37-orchestrator}
snap() { for x in status.md LIVE_BRANCHES; do if [ -e "$LIVE/$x" ]; then echo "$x $(stat -c %Y "$LIVE/$x") $(sha256sum < "$LIVE/$x" | cut -c1-16)"; else echo "$x none"; fi; done; }
if [ ! -d "$LIVE" ]; then ok "live folder $LIVE not on this machine: case 6 not needed"
else
  tries=0; res=changed
  while [ $tries -lt 2 ] && [ $res = changed ]; do
    tries=$((tries + 1)); a=$(snap)
    for t in test_status_lanes.sh test_status_checks.sh; do bash "$TOP/tests/host/$t" >/dev/null 2>&1; done
    b=$(snap); [ "$a" = "$b" ] && res=same
  done
  if [ $res = same ]; then ok "live status.md and LIVE_BRANCHES not changed by the status tests ($tries run)"; else bad "the status tests changed the live files in $LIVE: before [$a] after [$b]"; fi
fi
echo "status-isolation: $((n - fails)) of $n ok"
[ $fails = 0 ]
