#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Pre-install gate for the SMK-37 Elite firmware (Linux x86-64 box only). It does not touch a device.
#   tools/gate.sh <commit or ref> [--force]
# The gate makes its own worktree of the commit, and then it does these steps:
#   1. the Elite build, the Pro build (build/x0x-smk37.fwsc, for the package tests), the FM-1 build and
#      the Bluetooth image build (private HOME and private /tmp),
#   2. tests/run_tests.sh with AC79_SDK and the reference variables (parallel if the tree has X0X_TEST_JOBS),
#   3. the flash-map check (tools/gate_flashmap.py), the budget checks and a CRC manifest.
# It writes one result file per tree to $X0X_RESULTS/<tree>.md and adds a line to $X0X_RESULTS/index.md.
# The tree is a SHA-1 of "git ls-tree -r" without the *.md files. If a result for that tree exists,
# the gate uses it again and does not build (--force builds again).
# Output: one line, "PASS <hash>" or "FAIL <hash> <reason>". Exit code 0 or 1. The full log is in the worktree folder.
# Environment (defaults in brackets):
#   X0X_REPO [/workspace/x0x37.git]  X0X_RESULTS [/workspace/x0x37-results]  X0X_REFS [/workspace/x0x37-refs/schwung]
#   X0X_GATE_WORK [~/.cache/x0x37-gate]  X0X_TEST_JOBS [8]  JIELI_TOOLCHAIN [~/.jieli/toolchain]  AC79_SDK [~/fw-AC79_AIoT_SDK]
set -u
GATE_DIR=$(cd "$(dirname "$0")" && pwd)
REPO=${X0X_REPO:-/workspace/x0x37.git}
RESULTS=${X0X_RESULTS:-/workspace/x0x37-results}
REFS=${X0X_REFS:-/workspace/x0x37-refs/schwung}
WORK=${X0X_GATE_WORK:-$HOME/.cache/x0x37-gate}
JOBS=${X0X_TEST_JOBS:-8}
TC=${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}
SDK=${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}
FORCE=0
[ "${2:-}" = --force ] && FORCE=1
[ -n "${1:-}" ] || { echo "usage: tools/gate.sh <commit or ref> [--force]" >&2; exit 2; }

H=$(git -C "$REPO" rev-parse --verify --quiet "$1^{commit}") || { echo "FAIL $1 unknown commit in $REPO"; exit 1; }
S=${H:0:7}
TREE=$(git -C "$REPO" ls-tree -r --full-tree "$H" | awk -F '\t' '$2 !~ /\.md$/' | sha1sum | cut -c1-40)
NOW=$(date '+%Y-%m-%d %H:%M %Z')
mkdir -p "$RESULTS" && chmod a+rx "$RESULTS"
INDEX="$RESULTS/index.md"
[ -f "$INDEX" ] || printf '# x0x37 gate results: index\n\nEach line gives the commit, the tree (SHA-1 of the file list without *.md), the result and the time.\n\n| Commit | Tree | Result | Result file | Time |\n|---|---|---|---|---|\n' > "$INDEX"
chmod a+r "$INDEX"
RF="$RESULTS/$TREE.md"

if [ "$FORCE" = 0 ] && [ -f "$RF" ]; then
    R=$(sed -n 's/^Result: \([A-Z]*\).*/\1/p' "$RF" | head -1)
    WHY=$(sed -n 's/^Reason: //p' "$RF" | head -1)
    echo "| $S | ${TREE:0:12} | $R (result used again) | $TREE.md | $NOW |" >> "$INDEX"
    printf '\nThe gate used this result again for commit %s at %s. The tree is the same.\n' "$H" "$NOW" >> "$RF"
    if [ "$R" = PASS ]; then echo "PASS $S (result used again: tree ${TREE:0:12} has a result)"; exit 0; fi
    echo "FAIL $S ${WHY:-see $RF} (result used again: tree ${TREE:0:12})"; exit 1
fi

mkdir -p "$WORK"
[ -d "$WORK/.repo" ] || git clone -q --no-checkout "$REPO" "$WORK/.repo"
git -C "$WORK/.repo" fetch -q --update-shallow origin '+refs/heads/*:refs/remotes/origin/*' 2>/dev/null
git -C "$WORK/.repo" cat-file -e "$H^{commit}" 2>/dev/null || git -C "$WORK/.repo" fetch -q origin "$H"
W="$WORK/$S"
LOCK="$WORK/.lock-$S"
mkdir "$LOCK" 2>/dev/null || { echo "FAIL $S another gate run uses $W"; exit 1; }
trap 'rmdir "$LOCK" 2>/dev/null' EXIT
if [ -d "$W" ]; then
    git -C "$W" checkout -q --detach "$H" && git -C "$W" reset -q --hard "$H"
else
    git -C "$WORK/.repo" worktree prune
    git -C "$WORK/.repo" worktree add -q --detach "$W" "$H"
fi
L="$WORK/$S.logs"; rm -rf "$L"; mkdir -p "$L"
LOG="$L/gate.log"
T0=$(date +%s)
REASON=""
fail() { [ -n "$REASON" ] || REASON="$1"; echo "gate: FAIL $1" >> "$LOG"; }

cd "$W" || exit 1
for d in "$REFS"/schwung-*; do [ -d "$d" ] || fail "no reference sources in $REFS"; done
ls -d "$WORK"/schwung-* >/dev/null 2>&1 && fail "../schwung-* exists next to the worktree"
# Fallback: an older tests/host/run_drum909.sh copies the 9W9 sources with their read-only modes and then
# writes into the copy. For such a tree, use a writable copy of the references for this run only.
if [ -f tests/host/run_drum909.sh ] && ! grep -q 'chmod -R u+w "$OUT/ref_src"' tests/host/run_drum909.sh; then
    rm -rf "$L/refs"; mkdir -p "$L/refs"
    cp -r --no-preserve=mode "$REFS"/schwung-* "$L/refs/" && chmod -R u+w "$L/refs" && REFS="$L/refs"
    echo "gate: the tree has the old run_drum909.sh; the tests use the writable copy $REFS" >> "$LOG"
fi
export AC79_SDK="$SDK" JIELI_TOOLCHAIN="$TC"
export NINEW9_DIR="$REFS/schwung-9W9" EIGHTW8_DIR="$REFS/schwung-8W8" SCHWUNG303="$REFS/schwung-303"
export TB3PO_REF="$REFS/schwung-tb3po/src/dsp/tb3po.c"

# 1. builds
MODEL=elite ./build.sh --board smk37 > "$L/build-elite.log" 2>&1 || fail "Elite build: $(grep -m1 -iE 'error|fail' "$L/build-elite.log")"
./build.sh --board smk37 > "$L/build-pro.log" 2>&1 || fail "Pro build: $(grep -m1 -iE 'error|fail' "$L/build-pro.log")"
./build.sh > "$L/build-fm1.log" 2>&1 || fail "FM-1 build: $(grep -m1 -iE 'error|fail' "$L/build-fm1.log")"
PT="$L/ctl-tmp"; PH="$L/ctl-home"; mkdir -p "$PT" "$PH"
unshare -rm sh -c "mount --bind '$PT' /tmp && env HOME='$PH' JIELI_TOOLCHAIN='$TC' SDK_LOCAL='$SDK' CTL_NATIVE=1 sh ble/ctl/build_ctl.sh" \
    > "$L/build-ctl.log" 2>&1 || fail "ctl build: $(grep -m1 -iE 'error|fail|undefined' "$L/build-ctl.log")"

# 2. tests
X0X_TEST_JOBS="$JOBS" sh tests/run_tests.sh > "$L/tests.log" 2>&1; TRC=$?
OK=$(grep -c '^  ok ' "$L/tests.log"); NF=$(grep -c '^  FAIL' "$L/tests.log"); NS=$(grep -c '^  skip' "$L/tests.log")
[ "$TRC" = 0 ] || fail "tests: $NF failed ($(grep -m1 '^  FAIL' "$L/tests.log" | sed 's/^  FAIL //'))"
[ "$NS" = 0 ] || fail "tests: $NS skipped"
BLE=$(tail -n1 build/host/ble.log 2>/dev/null)
CK=$(tail -n1 build/host/smk-clip-keys.log 2>/dev/null)
SCEN=$(grep -c '^  ok   scenario-' "$L/tests.log")

# 3. flash map, budgets, CRCs
python3 "$GATE_DIR/gate_flashmap.py" firmware/hal/fm1_flash_map.h > "$L/flashmap.log" 2>&1 || fail "$(tail -n1 "$L/flashmap.log")"
FM=$(tail -n1 "$L/flashmap.log")
num() { grep -m1 -oE "$1" "$2" | grep -oE '[0-9]+' | sed -n "${3}p"; }
CODE_FREE=$(num 'app code area [0-9]+ B, [0-9]+ B free' "$L/build-elite.log" 2)
RAM_USED=$(num 'RAM \.data\+\.bss [0-9]+ B of [0-9]+' "$L/build-elite.log" 1)
RAM_MAX=$(num 'RAM \.data\+\.bss [0-9]+ B of [0-9]+' "$L/build-elite.log" 2)
POOL=$(num 'pool [0-9]+ B of [0-9]+' "$L/build-elite.log" 1)
POOL_MAX=$(num 'pool [0-9]+ B of [0-9]+' "$L/build-elite.log" 2)
CTL_LINE=$(grep -m1 'ctl.img: ' "$L/build-ctl.log")
CTL_SIZE=$(echo "$CTL_LINE" | grep -oE ': [0-9]+ B of [0-9]+' | grep -oE '[0-9]+' | head -1)
CORE=$(echo "$CTL_LINE" | grep -oE '\([0-9]+ B of [0-9]+\)' | head -1 | grep -oE '[0-9]+')
CORE_USED=$(echo "$CORE" | sed -n 1p); CORE_MAX=$(echo "$CORE" | sed -n 2p)
POOL_HEAD=$(( ${POOL_MAX:-0} - ${POOL:-0} )); RAM_FREE=$(( ${RAM_MAX:-0} - ${RAM_USED:-0} ))
CORE_FREE=$(( ${CORE_MAX:-0} - ${CORE_USED:-0} ))
[ -n "$POOL" ] && [ "$POOL_HEAD" -ge 8192 ] || fail "pool headroom ${POOL_HEAD} B < 8192 B"
[ -n "$RAM_USED" ] && [ "$RAM_FREE" -ge 0 ] || fail "app RAM over the limit"
[ -n "$CODE_FREE" ] || fail "app code over the limit"
[ -n "$CTL_SIZE" ] && [ "$CTL_SIZE" -le 65536 ] || fail "ctl.img is not one piece of 65536 B or less"
[ -n "$CORE_USED" ] && [ "$CORE_FREE" -ge 0 ] || fail "core RAM does not fit"
python3 - "$L/crc.txt" <<'PY' || fail "CRC manifest"
import glob, struct, sys, zlib
lines = []
def crc(p): return format(zlib.crc32(open(p, "rb").read()), "08x")
img = "ble/ctl/out/ctl.img"
d = open(img, "rb").read()
magic, hv, hs, fw, size, hcrc = struct.unpack("<IHHIII", d[:20])
lines.append(f"ctl.img header crc32 {hcrc:08x}, image_size {size} B")
lines.append(f"ctl.img file crc32 {crc(img)}, {len(d)} B")
for p in sorted(glob.glob("build/*.fwsc") + glob.glob("build/x0x.bin") + glob.glob("build/*/x0x.bin")
                + glob.glob("build/loader/ota.bin") + glob.glob("build/*/loader/ota.bin")):
    lines.append(f"{p} crc32 {crc(p)}, {len(open(p, 'rb').read())} B")
open(sys.argv[1], "w").write("\n".join(lines) + "\n")
PY
T1=$(date +%s)
R=PASS; [ -n "$REASON" ] && R=FAIL
TMPF="$RF.tmp.$$"
{
echo "# Gate result for tree $TREE"
echo
echo "This file uses ASD Simplified Technical English."
echo
echo "Result: $R"
[ -n "$REASON" ] && echo "Reason: $REASON"
echo
echo "- Commit: $H"
echo "- Tree (SHA-1 of git ls-tree -r without *.md): $TREE"
echo "- Time: $NOW. The gate took $((T1 - T0)) s."
echo "- Worktree: $W. Logs: $L."
echo
echo "## Builds"
echo
echo "- Elite build: $(grep -m1 '^package' "$L/build-elite.log" | sed "s|$W/||")"
echo "- Pro build: $(grep -m1 '^package' "$L/build-pro.log" | sed "s|$W/||")"
echo "- FM-1 build: $(grep -m1 '^package' "$L/build-fm1.log" | sed "s|$W/||")"
echo "- Bluetooth image: $(echo "$CTL_LINE" | sed "s|$W/||")"
echo
echo "## Sizes"
echo
echo "| Item | Value |"
echo "|---|---|"
echo "| App code free | ${CODE_FREE:-?} B |"
echo "| App RAM (.data+.bss) | ${RAM_USED:-?} of ${RAM_MAX:-?} B, $RAM_FREE B free |"
echo "| Pool | ${POOL:-?} of ${POOL_MAX:-?} B. Pool headroom $POOL_HEAD B. The limit is 8192 B. |"
echo "| ctl.img | ${CTL_SIZE:-?} of 65536 B, $(( 65536 - ${CTL_SIZE:-0} )) B free |"
echo "| Core RAM | ${CORE_USED:-?} of ${CORE_MAX:-?} B, $CORE_FREE B free |"
echo
echo "## Tests"
echo
echo "- tests/run_tests.sh (X0X_TEST_JOBS=$JOBS): $OK ok, $NF FAIL, $NS skip. Exit code $TRC."
echo "- Scenarios: $SCEN ok."
echo "- ble_test: ${BLE:-no log}"
echo "- smk_clip_keys_test: ${CK:-no log}"
echo
echo "## Flash map"
echo
echo '```'
cat "$L/flashmap.log"
echo '```'
echo
echo "## CRC manifest"
echo
echo '```'
cat "$L/crc.txt" 2>/dev/null
echo '```'
} > "$TMPF"
mv "$TMPF" "$RF"; chmod a+r "$RF"
echo "| $S | ${TREE:0:12} | $R | $TREE.md | $NOW |" >> "$INDEX"
cp "$RF" "$L/result.md"
if [ "$R" = PASS ]; then echo "PASS $S"; exit 0; fi
echo "FAIL $S $REASON"; exit 1
