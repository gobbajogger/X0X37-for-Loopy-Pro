#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Run mutations only in a private source copy; compiler errors are not killed mutants.
set -eu
cd "$(dirname "$0")/../.."
work=$(mktemp -d "${TMPDIR:-/tmp}/x0x-panic-mutants.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/tests/host" "$work/firmware/src/app" "$work/host"
cp tests/host/live_key_epoch_test.py tests/host/midi_epoch_step_test.py "$work/tests/host/"
cp firmware/src/app/engine.c "$work/firmware/src/app/"
cp host/x0x_host.c "$work/host/"
cd "$work"
python3 tests/host/live_key_epoch_test.py > baseline-live.log 2>&1 || { cat baseline-live.log; echo 'BASELINE FAILED: live-key'; exit 1; }
python3 tests/host/midi_epoch_step_test.py > baseline-midi.log 2>&1 || { cat baseline-midi.log; echo 'BASELINE FAILED: midi-epoch'; exit 1; }
python3 - <<'PY'
import pathlib
p=pathlib.Path('firmware/src/app/engine.c');s=p.read_text()
old='if(o->blocked || c.generation!=o->generation)continue;'
if s.count(old)!=1: raise SystemExit('mutation anchor not unique')
p.write_text(s.replace(old,'if(o->blocked)continue;',1))
PY
rc=0
python3 tests/host/live_key_epoch_test.py > stale-generation.log 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then echo 'MUTANT SURVIVED: stale-generation'; exit 1; fi
if [ "$rc" -ne 1 ] || ! grep -qx 'FAIL live key epoch: applied=2 pitch=61 gate=1 r=2 w=2' stale-generation.log; then
    cat stale-generation.log; echo 'MUTANT ERROR: stale-generation'; exit 1
fi
echo 'MUTANT KILLED: stale-generation'
python3 - <<'PY'
import pathlib
p=pathlib.Path('host/x0x_host.c');s=p.read_text();old='if (!++mi_epoch)'
if s.count(old)!=1: raise SystemExit('MIDI epoch mutation anchor not unique')
p.write_text(s.replace(old,'if (!(mi_epoch += 2u))',1))
PY
rc=0
python3 tests/host/midi_epoch_step_test.py > midi-epoch-plus2.log 2>&1 || rc=$?
if [ "$rc" -eq 0 ]; then echo 'MUTANT SURVIVED: midi-epoch-plus2'; exit 1; fi
if [ "$rc" -ne 1 ] || ! grep -qx 'FAIL MIDI epoch sequence: 1 3 5' midi-epoch-plus2.log; then
    cat midi-epoch-plus2.log; echo 'MUTANT ERROR: midi-epoch-plus2'; exit 1
fi
echo 'MUTANT KILLED: midi-epoch-plus2'
echo 'panic mutants: 2 killed'
