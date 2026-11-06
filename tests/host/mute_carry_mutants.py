#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run only in an isolated checkout: restore each production file after its test.
Compile errors do not count as killed mutants; require the behavioral assertion.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SEQ = 'firmware/src/seq/sequencer.c'
ENG = 'firmware/src/app/engine.c'
CASES = [
    ('mute-releases-voice', SEQ, 'if (muted) bass_midi_flush(s, b, o);',
     'if (muted) { bass_midi_flush(s, b, o); gate = G_REST; }',
     ['sh', 'tests/host/seq_mute_carry_test.sh'], 'seq.t[t].sounding'),
    ('mute-leaks-midi', SEQ, 'played && !muted && s->send_notes',
     'played && s->send_notes', ['sh', 'tests/host/seq_mute_carry_test.sh'],
     '!saw_status(0x90 | seq.ch[t])'),
    ('mute-keeps-owned-midi', SEQ, 'if (muted) bass_midi_flush(s, b, o);',
     'if (muted) (void)0;', ['sh', 'tests/host/seq_mute_carry_test.sh'],
     '!seq.bass_out_valid[b]'),
    ('mute-bypasses-off', SEQ, '|| SEQ_PART_OFF(s, TRK_BASS0 + b)',
     '|| (SEQ_PART_OFF(s, TRK_BASS0 + b) && !muted)',
     ['sh', 'tests/host/seq_mute_carry_test.sh'], '!seq.t[t].sounding'),
    ('bus-gate-removed', ENG,
     'if ((p == PART_303A || p == PART_303B) && seq_part_muted(&seq, p))',
     'if (0)', ['sh', 'tests/host/synth_bus_mute_test.sh'],
     'render_energy(100)<audible/100'),
    ('break-boundary-removed', ENG,
     'brk_mute_boundary(); /* also follows song mutes at an in-buffer event */',
     '/* mutant: no immediate BREAK mute */',
     ['python3', 'tests/host/engine_brk_mute_test.py'],
     'mute bypasses full CQ'),
]

def main():
    with tempfile.TemporaryDirectory(prefix='mute-carry-mutants-') as directory:
        for name, relative, old, new, command, expected in CASES:
            path = ROOT / relative
            original = path.read_bytes()
            source = original.decode()
            if source.count(old) != 1:
                raise SystemExit(f'{name}: ambiguous mutation anchor')
            try:
                path.write_text(source.replace(old, new))
                result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
            finally:
                path.write_bytes(original)
            log = result.stdout + result.stderr
            (Path(directory) / (name + '.log')).write_text(log)
            if result.returncode == 0 or expected not in log or 'error:' in log:
                print(log)
                raise SystemExit(f'{name}: survived or invalid failure')
            print(f'KILLED {name}: {expected}', flush=True)
    print('mute carry mutants: 6 killed')

if __name__ == '__main__':
    main()
