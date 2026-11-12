#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Mutate the 25.1 live-key generation guard; restore the exact source afterward.
set -eu
cd "$(dirname "$0")/../.."
file=firmware/src/app/engine.c
backup=$(mktemp)
cp "$file" "$backup"
restore() { cp "$backup" "$file"; }
trap 'restore; rm -f "$backup"' EXIT HUP INT TERM
python3 - "$file" <<'PY'
import pathlib,sys
p=pathlib.Path(sys.argv[1]);s=p.read_text()
old='if(o->blocked || c.generation!=o->generation)continue;'
new='if(o->blocked)continue;'
if s.count(old)!=1: raise SystemExit("mutation anchor not unique")
p.write_text(s.replace(old,new,1))
PY
set +e
python3 tests/host/live_key_epoch_test.py > /tmp/x0x-panic-stale-generation.log 2>&1
rc=$?
set -e
restore
if [ "$rc" -eq 0 ]; then echo "MUTANT SURVIVED: stale-generation"; exit 1; fi
echo "MUTANT KILLED: stale-generation"
midi_file=host/x0x_host.c
midi_backup=$(mktemp)
cp "$midi_file" "$midi_backup"
python3 - "$midi_file" <<'PY'
import pathlib,sys
p=pathlib.Path(sys.argv[1]);s=p.read_text()
old='if (!++mi_epoch)'
new='if (!(mi_epoch += 2u))'
if s.count(old)!=1: raise SystemExit("MIDI epoch mutation anchor not unique")
p.write_text(s.replace(old,new,1))
PY
set +e
python3 tests/host/midi_epoch_step_test.py > /tmp/x0x-panic-midi-epoch-plus2.log 2>&1
midi_rc=$?
set -e
cp "$midi_backup" "$midi_file"; rm -f "$midi_backup"
if [ "$midi_rc" -eq 0 ]; then echo "MUTANT SURVIVED: midi-epoch-plus2"; exit 1; fi
echo "MUTANT KILLED: midi-epoch-plus2"
echo "panic mutants: 2 killed"
