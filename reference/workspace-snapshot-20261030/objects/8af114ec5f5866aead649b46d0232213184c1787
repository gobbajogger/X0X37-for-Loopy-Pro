#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-only
# Install the gate (tools/gate.sh as x0x37_gate.sh, tools/gate_sigwrap.py, tools/gate_flashmap.py, tools/validation_inputs.py, tools/prepush-check.sh, tools/quickcheck.sh into the shared gate folder.
#   tools/install_gate.sh
# The install starts only when the gate lock is free (flock -n) AND no x0x37_gate.sh process runs (pgrep).
# It holds the lock during the install. Each file goes to a temp file in the same folder, gets mode 555 and
# replaces the old file with "mv -f" (a running bash never reads a half-written script). The folder mode is
# restored after the install. If the gate is busy, the script waits (5 s steps, at most X0X_INSTALL_WAIT seconds)
# and then stops with "NOT INSTALLED" and the reason. Exit code 0 (installed) or 1.
# Environment (defaults in brackets): X0X_GATE_DEST [/workspace/x0x37-refs/gate]
#   X0X_GATE_LOCK [/workspace/x0x37-refs/gate.lock]  X0X_INSTALL_WAIT [1800]
set -u
SRC=$(cd "$(dirname "$0")" && pwd)
DEST=${X0X_GATE_DEST:-/workspace/x0x37-refs/gate}
GLOCK=${X0X_GATE_LOCK:-/workspace/x0x37-refs/gate.lock}
MAX=${X0X_INSTALL_WAIT:-1800}
for f in gate.sh gate_sigwrap.py gate_flashmap.py validation_inputs.py prepush-check.sh quickcheck.sh quickcheck_plan.py pre-push.hook; do [ -f "$SRC/$f" ] || { echo "install_gate: NOT INSTALLED: no $SRC/$f"; exit 1; }; done
bash -n "$SRC/gate.sh" || { echo "install_gate: NOT INSTALLED: syntax error in $SRC/gate.sh"; exit 1; }
[ -d "$DEST" ] || { echo "install_gate: NOT INSTALLED: no folder $DEST"; exit 1; }
exec 9>>"$GLOCK" || { echo "install_gate: NOT INSTALLED: cannot open $GLOCK"; exit 1; }
T0=$(date +%s); SAID=""
while :; do
    WHY=""
    if ! flock -n 9; then WHY="the gate lock $GLOCK is held"
    else
        RUN=$(pgrep -af '/?x0x37_gate\.sh( |$)' | grep -v -e pgrep -e install_gate)
        [ -z "$RUN" ] || { WHY="a gate runs: $(echo "$RUN" | head -1 | cut -c1-120)"; flock -u 9; }
    fi
    [ -z "$WHY" ] && break
    if [ $(( $(date +%s) - T0 )) -ge "$MAX" ]; then
        echo "install_gate: NOT INSTALLED after $(( $(date +%s) - T0 )) s: $WHY"; exit 1
    fi
    [ "$SAID" = "$WHY" ] || { echo "install_gate: waiting ($WHY)" >&2; SAID=$WHY; }
    sleep 5
done
# the lock is held and no gate runs
P0=$(stat -c %a "$DEST")
chmod u+w "$DEST" || { echo "install_gate: NOT INSTALLED: cannot write $DEST"; exit 1; }
ok=1
for p in "gate.sh x0x37_gate.sh" "gate_sigwrap.py gate_sigwrap.py" "gate_flashmap.py gate_flashmap.py" "validation_inputs.py validation_inputs.py" "prepush-check.sh prepush-check.sh" "quickcheck.sh quickcheck.sh" "quickcheck_plan.py quickcheck_plan.py" "pre-push.hook pre-push.hook"; do
    set -- $p
    t="$DEST/.$2.new.$$"
    cp "$SRC/$1" "$t" && chmod 555 "$t" && mv -f "$t" "$DEST/$2" && cmp -s "$SRC/$1" "$DEST/$2" || { ok=0; rm -f "$t"; }
done
chmod "$P0" "$DEST"
flock -u 9; exec 9>&-
[ "$ok" = 1 ] || { echo "install_gate: FAILED: a file did not install (see $DEST)"; exit 1; }
echo "install_gate: INSTALLED $(sha256sum "$DEST/x0x37_gate.sh" | cut -c1-12) x0x37_gate.sh, $(sha256sum "$DEST/gate_sigwrap.py" | cut -c1-12) gate_sigwrap.py, $(sha256sum "$DEST/gate_flashmap.py" | cut -c1-12) gate_flashmap.py, $(sha256sum "$DEST/prepush-check.sh" | cut -c1-12) prepush-check.sh, $(sha256sum "$DEST/quickcheck.sh" | cut -c1-12) quickcheck.sh in $DEST (folder mode $P0, waited $(( $(date +%s) - T0 )) s)"
