#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# tools/lane_files.py and tools/update.hook part 1e (checks 31 and 8 of rules v4, commit.md rule 10: lane files).
#   Parser: the "Lane files" section of smk37 38ff65d (fixture map-38ff65d.md). "(but not ...)", "dir/**" = dir too, a
#     row wins over a default, shared rows give each lane. A bad row or no section: LANES ERROR, exit 2.
#   Check 31: the 439 files of integration fe95e74 all have a lane (LANES OK, one WARN for tests/host/smk_clip_play_ports_*
#     that matches no file). Two new files with no lane: LANES FAIL, exit 1, the two files in the output.
#   Check 8 (lane-file half): a temp bare repository with the hooks of this tree, the map on smk37. A lane push that changes
#     a file that only another lane owns is rejected; own files, shared files, a file with no lane, a lane on smk37, an old
#     branch, an archive branch and a delete pass. No lane_files.py or no map on smk37: rejected (fail closed).
#   Check 31 in tools/quickcheck.sh (lanes step, X0X_QUICK_ONLY=ste): a commit that adds a file with no lane gives
#     "QUICK FAIL ... LANES: ...", a commit with lane files passes, a smk37 commit that changes the map is checked
#     against the files of integration (a map without the mb default fails).
#   Check 31 in tools/status.sh: "Lane files (check 31): 5 of 5 files have a lane", then NOTES.txt on integration gives
#     "**LANE FILES** (check 31): 1 of 6 files map to no lane: NOTES.txt".
# Fixtures: tests/host/fixtures/lane_files/.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; LF="$TOP/tools/lane_files.py"; F="$TOP/tests/host/fixtures/lane_files"
T=$(mktemp -d "${TMPDIR:-/tmp}/lanefiles.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
res() { n=$((n + 1)); if [ "$1" = 0 ]; then echo "ok   $2"; else echo "FAIL $2"; [ -n "${3:-}" ] && printf '%s\n' "$3" | sed 's/^/     /'; fails=$((fails + 1)); fi; }
M="$F/map-38ff65d.md"
# parser: the lanes of each path
while read -r p want; do
    case "$p" in ''|'#'*) continue;; esac
    got=$(python3 "$LF" of --map-file "$M" "$p" 2>&1 | sed 's/^[^:]*: //; s/ (.*//'); g=$(echo "$got" | tr ' ' '-'); [ "$g" = "no-lane" ] && g=none
    [ "$g" = "$want" ]; res $? "lanes of $p = $want" "got: $got"
done < "$F/of-cases.txt"
for m in map-bad-row.md map-none.md; do
    o=$(python3 "$LF" cover --map-file "$F/$m" --files "$F/files-fe95e74.txt" 2>&1); rc=$?
    [ $rc = 2 ] && echo "$o" | grep -q '^LANES ERROR'; res $? "$m: LANES ERROR, exit 2" "rc=$rc $o"
done
# check 31
o=$(python3 "$LF" cover --map-file "$M" --files "$F/files-fe95e74.txt" 2>&1); rc=$?
[ $rc = 0 ] && echo "$o" | grep -q '^LANES OK 439 of 439 files'; res $? "31: fe95e74, 439 of 439 files have a lane" "rc=$rc $o"
[ "$(echo "$o" | grep -c '^LANES WARN')" = 1 ] && echo "$o" | grep -qF 'LANES WARN: tests/host/smk_clip_play_ports_* (owned mb) matches no file'; res $? "31: play_ports pattern is a WARN only" "$o"
{ cat "$F/files-fe95e74.txt"; echo NOTES.txt; echo scripts/run.sh; } > "$T/files2.txt"
o=$(python3 "$LF" cover --map-file "$M" --files "$T/files2.txt" 2>&1); rc=$?
[ $rc = 1 ] && echo "$o" | grep -qE '^LANES FAIL 2 of 441 files map to no lane .*: NOTES.txt scripts/run.sh$'; res $? "31: 2 files with no lane, FAIL lists them" "rc=$rc $o"
# check 8, lane-file half, in the update hook
B="$T/box.git"; git init -q --bare "$B"
printf '#!/bin/bash\nste_step() { :; }\necho "QUICK PASS stand-in"\n' > "$T/qc.sh"; chmod +x "$T/qc.sh"
for k in "workDir $T/wd" "skipLog $T/skips.log" "quickcheck $T/qc.sh" "gateLock $T/gate.lock" "laneFiles $LF"; do set -- $k; git -C "$B" config "x0x37.$1" "$2"; done
C="$T/c"; git init -q -b integration "$C"; gc() { git -C "$C" -c user.name=t -c user.email=t@t commit -q "$@"; }
mkdir -p "$C/firmware/src/app" "$C/tests" "$C/docs/rules" "$C/tools"
for f in firmware/src/app/ui.c firmware/src/x.c BEHAVIOR.md tests/run_tests.sh README.md; do echo base > "$C/$f"; done
git -C "$C" add -A; gc -m base
git -C "$C" checkout -q -b smk37; mkdir -p "$C/docs"; cp "$M" "$C/docs/AGENTS_AND_ROOMS.md"; echo r > "$C/docs/rules/commit.md"; git -C "$C" add -A; gc -m docs
git -C "$C" checkout -q integration; git -C "$C" branch ble-old
git -C "$C" push -q "$B" integration smk37 ble-old 2>/dev/null || { echo "FAIL setup push"; exit 1; }
bash "$TOP/tools/install_server_hooks.sh" "$B" >/dev/null || { echo "FAIL hooks"; exit 1; }
lp() {  # lp <accept|reject> <name> <branch> <from> <file ...>: a commit on <from> that changes the files, pushed to <branch>
    local want=$1 name=$2 br=$3 from=$4; shift 4
    git -C "$C" checkout -q -B "$br" "$from"; for f in "$@"; do mkdir -p "$C/$(dirname "$f")"; echo "$name" >> "$C/$f"; done
    git -C "$C" add -A; gc -m "$name"
    out=$(git -C "$C" push -f "$B" "$br" 2>&1); rc=$?
    if [ "$want" = accept ]; then [ $rc = 0 ]; else [ $rc != 0 ] && echo "$out" | grep -q 'REJECTED'; fi
    res $? "8: $want: $name ($(echo "$out" | grep -m1 -oE 'REJECTED.*|LANE OK.*' | cut -c1-110))" "$out"
}
lp accept "lane/sb changes ui.c (sb owns it)" lane/sb-ui integration firmware/src/app/ui.c
lp reject "lane/opt changes ui.c (sb owns it)" lane/opt-ui integration firmware/src/app/ui.c
lp reject "lane/btr changes firmware/src/x.c (mb default)" lane/btr-fw integration tools/new.py firmware/src/x.c
lp accept "lane/mb changes BEHAVIOR.md (shared dk, mb, sb)" lane/mb-beh integration BEHAVIOR.md firmware/src/x.c
lp accept "lane/btr changes tests/run_tests.sh (shared) and tools/new.py" lane/btr-tools integration tests/run_tests.sh tools/new.py
lp accept "lane/mb adds NOTES.txt (no lane: check 31 reports it)" lane/mb-notes integration NOTES.txt
lp accept "lane/dk on smk37 changes docs/rules/commit.md" lane/dk-rules-x smk37 docs/rules/commit.md
lp reject "lane/dk changes README.md (mb owns it)" lane/dk-readme integration README.md
lp accept "old branch ble-old changes ui.c (not a lane ref)" ble-old integration firmware/src/app/ui.c
git -C "$C" branch -f archive/lane/opt-ui lane/opt-ui
out=$(git -C "$C" push "$B" archive/lane/opt-ui 2>&1); res $? "8: accept: archive/lane/opt-ui (not a lane ref)" "$out"
out=$(git -C "$C" push "$B" :lane/sb-ui 2>&1); res $? "8: accept: delete lane/sb-ui" "$out"
git -C "$B" config x0x37.laneFiles "$T/missing.py"
lp reject "no lane_files.py: fail closed" lane/sb-ui2 integration firmware/src/app/ui.c
git -C "$B" config x0x37.laneFiles "$LF"
git -C "$C" checkout -q -B smk37 smk37; cp "$F/map-none.md" "$C/docs/AGENTS_AND_ROOMS.md"; git -C "$C" add -A; gc -m "No map"
git -C "$C" push -q "$B" smk37 2>/dev/null
lp reject "no Lane files section on smk37: fail closed" lane/sb-ui3 integration firmware/src/app/ui.c
# check 31 in quickcheck.sh
R="$T/repo"; git clone -q "$B" "$R" 2>/dev/null; git -C "$R" checkout -q integration
qlane() {  # qlane <branch> <from> <file> [map file]: a commit, its hash
    git -C "$R" checkout -q -B "$1" "$2"; mkdir -p "$R/$(dirname "$3")"
    if [ -n "${4:-}" ]; then cp "$4" "$R/$3"; else echo x >> "$R/$3"; fi
    git -C "$R" add -A; git -C "$R" -c user.name=t -c user.email=t@t commit -qm "Change $3." -m "The file has a new line."; git -C "$R" rev-parse HEAD; }
git -C "$R" checkout -q smk37 && cp "$M" "$R/docs/AGENTS_AND_ROOMS.md" && git -C "$R" add -A && git -C "$R" -c user.name=t -c user.email=t@t commit -qm "Restore the map." -m "The map has the lanes." && git -C "$R" push -q origin smk37 2>/dev/null
sed '/^| firmware\/\*\*, web\/\*\* | mb |$/d' "$M" > "$T/map-nofw.md"
NOL=$(qlane lane/mb-q1 integration NOTES.txt); OKL=$(qlane lane/btr-q2 integration tools/q.py); MAPC=$(qlane lane/dk-q3 smk37 docs/AGENTS_AND_ROOMS.md "$T/map-nofw.md")
git -C "$R" push -q -f origin lane/mb-q1 lane/btr-q2 lane/dk-q3 2>/dev/null
: > "$T/pre-push"
qc() { X0X_QUICK_ONLY=ste X0X_REPO="$R" X0X_QC_WORK="$T/qc" X0X_PREPUSH_HOOK="$T/pre-push" X0X_QC_BASE="$2" bash "$TOP/tools/quickcheck.sh" "$1" 2>&1 | grep '^QUICK ' | tail -1; }
q=$(qc "$NOL" "$NOL^"); echo "$q" | grep -qE '^QUICK FAIL .*LANES: 1 of [0-9]+ files map to no lane: NOTES.txt$'; res $? "31 quickcheck: NOTES.txt has no lane: $q"
q=$(qc "$OKL" "$OKL^"); echo "$q" | grep -q '^QUICK PASS '; res $? "31 quickcheck: tools/q.py (btr default): $q"
q=$(qc "$MAPC" "$MAPC^"); echo "$q" | grep -qE '^QUICK FAIL .*LANES: .*firmware/src/x.c'; res $? "31 quickcheck: a map change without the mb default: $q"
# check 31 in status.sh (B: smk37 has the map again, from the quickcheck part)
export X0X_TEST_RUN=1
st() { rm -rf "$T/out"; mkdir -p "$T/out"; X0X_REPO="$B" X0X_STATUS_OUT="$T/out" X0X_RESULTS_DIR="$T/res" X0X_GATE_LOCK="$T/gate.lock" bash "$TOP/tools/status.sh" >/dev/null 2>&1; grep -E '^- .*\(check 31\)' "$T/out/status.md"; }
mkdir -p "$T/res"; o=$(st); echo "$o" | grep -qF -- "- Lane files (check 31): 5 of 5 files have a lane"; res $? "31 status: $o"
git -C "$R" checkout -q integration && echo n > "$R/NOTES.txt" && git -C "$R" add -A && git -C "$R" -c user.name=t -c user.email=t@t commit -qm "Add notes." && git -C "$R" push -q -o by=merge-integrator origin integration 2>/dev/null
o=$(st); echo "$o" | grep -qF -- "- **LANE FILES** (check 31): 1 of 6 files map to no lane: NOTES.txt."; res $? "31 status: $o"
echo "lane files: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
