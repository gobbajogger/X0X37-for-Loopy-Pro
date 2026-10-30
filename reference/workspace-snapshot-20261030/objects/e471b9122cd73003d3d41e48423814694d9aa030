#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# tools/update.hook parts 1c and 1d on a scratch bare repository with the hooks of this tree (tools/install_server_hooks.sh).
#   1c (check 9, commit.md rule 10): a new branch must be lane/<mb|sb|opt|btr|dk>-<topic>, integration, smk37 or archive/<name>.
#      Cases: tests/host/fixtures/update_names/branches.txt. A branch that was there before (ble-old) can move and be deleted.
#   1d (check 8, rule 10): a push to integration needs git push -o by=merge-integrator. No option, or another by=, is
#      rejected. The quick check of part 2 is a stand-in here (x0x37.quickcheck), so the test checks only the option.
set -u
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_OBJECT_DIRECTORY GIT_ALTERNATE_OBJECT_DIRECTORIES GIT_COMMON_DIR GIT_PREFIX GIT_QUARANTINE_PATH GIT_NAMESPACE
TOP="$(cd "$(dirname "$0")/../.." && pwd)"; F="$TOP/tests/host/fixtures/update_names/branches.txt"
T=$(mktemp -d "${TMPDIR:-/tmp}/updnames.XXXXXX"); trap 'rm -rf "$T"' EXIT
fails=0; n=0
B="$T/box.git"; C="$T/c"
printf '#!/bin/bash\n# stand-in quick check\nste_step() { :; }\necho "QUICK PASS stand-in"\n' > "$T/qc.sh"; chmod +x "$T/qc.sh"
git init -q --bare "$B" && git -C "$B" config x0x37.workDir "$T/wd" && git -C "$B" config x0x37.skipLog "$T/skips.log" &&
    git -C "$B" config x0x37.quickcheck "$T/qc.sh" && git -C "$B" config x0x37.gateLock "$T/gate.lock" || { echo "FAIL setup"; exit 1; }
gc() { git -C "$C" -c user.name=t -c user.email=t@t commit -q "$@"; }
git init -q -b integration "$C" && echo a > "$C/a.c" && git -C "$C" add . && gc -m base && git -C "$C" remote add box "$B" &&
    git -C "$C" branch ble-old && git -C "$C" push -q box integration ble-old 2>/dev/null || { echo "FAIL setup: first push"; exit 1; }
bash "$TOP/tools/install_server_hooks.sh" "$B" >/dev/null || { echo "FAIL setup: hooks"; exit 1; }
push() {  # push <accept|reject> <name> <git push args...>
    want=$1; name=$2; shift 2; n=$((n + 1))
    out=$(git -C "$C" push box "$@" 2>&1); rc=$?
    if { [ "$want" = accept ] && [ $rc = 0 ]; } || { [ "$want" = reject ] && [ $rc != 0 ] && echo "$out" | grep -q 'REJECTED'; }; then
        echo "ok   $name: $(echo "$out" | grep -m1 -E 'REJECTED|update hook' | sed 's/^remote: //' | cut -c1-110)"
    else echo "FAIL $name (want $want, rc=$rc)"; echo "$out" | sed 's/^/     /'; fails=$((fails + 1)); fi
}
while IFS='|' read -r want b; do
    case "$want" in "#"*|"") continue;; esac
    push "$want" "new branch $b" "integration:refs/heads/$b"
done < "$F"
echo b >> "$C/a.c"; gc -am "Change b"
push accept "an old branch (ble-old) moves" HEAD:ble-old
push accept "an old branch (ble-old) is deleted" :ble-old
push reject "integration with no push option" integration
push reject "integration with -o by=second-builder" -o by=second-builder integration
push accept "integration with -o by=merge-integrator" -o by=merge-integrator integration
echo c >> "$C/a.c"; gc -am "Change c"
push reject "integration with only a skip option" -o skip-quickcheck="test" integration
push accept "integration with -o by=merge-integrator and a skip" -o by=merge-integrator -o skip-quickcheck="test" integration
echo "update names: $((n - fails)) of $n ok"
[ "$fails" = 0 ]
