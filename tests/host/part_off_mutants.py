#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# M7a mutants for tests/host/run_part_off.sh: each is applied to a copy of the tree, never to the sources.
# A mutant is killed when the case it names FAILs. usage: part_off_mutants.py list | apply <name> <root>
import sys
E_C = 'firmware/src/app/engine.c'
S_C = 'firmware/src/seq/sequencer.c'
S_H = 'firmware/src/seq/sequencer.h'
P_C = 'firmware/src/app/project.c'
MUTANTS = {
    # (a) a BREAK pad release gated by OFF
    'brk_release_gated': (E_C, 'if (down && SEQ_PART_OFF(&seq, TRK_BRK)) return;',
                          'if (SEQ_PART_OFF(&seq, TRK_BRK)) return;', 'brk_pad_release_not_gated'),
    # (b) a MIDI Note Off gated by OFF: the break, then the 303s
    'midi_off_gated_brk': (E_C, 'if (!(ty == 0x90 && SEQ_PART_OFF(&seq, TRK_BRK)))',
                           'if (!SEQ_PART_OFF(&seq, TRK_BRK))', 'midi_note_off_not_gated_break'),
    'midi_off_gated_bass': (E_C, 'if (ty == 0x90 && SEQ_PART_OFF(&seq, TRK_BASS0 + k))',
                            'if (SEQ_PART_OFF(&seq, TRK_BASS0 + k))', 'midi_note_off_not_gated_chicago'),
    # (c) engine_bass_off_note / engine_bass_off gated by OFF
    'bass_off_note_gated': (E_C, 'void engine_bass_off_note(int part, int note)\n{\n',
                            'void engine_bass_off_note(int part, int note)\n{\n    if ((unsigned)part < NBASS && SEQ_PART_OFF(&seq, TRK_BASS0 + part)) return;\n',
                            'bass_off_note_not_gated_chicago'),
    'bass_off_gated': (E_C, 'void engine_bass_off(int part) { cq_put(',
                       'void engine_bass_off(int part) { if ((unsigned)part < NBASS && SEQ_PART_OFF(&seq, TRK_BASS0 + part)) return; cq_put(',
                       'live_303_held_off_release_chicago'),
    # (d) the sequencer 303 OFF path returning without bass_release (a held / TIE'd note sticks)
    'seq_bass_skip_release': (S_C, '    if (gate == G_REST || (gate == G_TIE && !tr->sounding) || SEQ_PART_OFF(s, TRK_BASS0 + b)) {',
                              '    if (SEQ_PART_OFF(s, TRK_BASS0 + b)) { tr->held_note = 0; return; }\n'
                              '    if (gate == G_REST || (gate == G_TIE && !tr->sounding)) {',
                              'seq_bass_tie_released_after_off_chicago'),
    # independence: OFF implemented as a mute, and a mute read as OFF
    'off_writes_mute': (P_C, '    engine_out_loud(proj.set.out_loud);\n',
                        '    for (p = 0; p < 5; p++) if (seq.part_off >> p & 1u) seq.mute |= seq_part_mask(p);\n'
                        '    engine_out_loud(proj.set.out_loud);\n', 'independent_of_mute_solo'),
    'mute_reads_as_off': (S_H, '#define SEQ_PART_OFF(s, t) (((s)->part_off >> (t)) & 1u)',
                          '#define SEQ_PART_OFF(s, t) ((((s)->part_off >> (t)) & 1u) | ((t) >= 2 && ((s)->mute >> (MUTE_BASS0 + (t) - 2) & 1u)))',
                          'independent_of_mute_solo'),
}
if sys.argv[1] == 'list':
    for n, m in MUTANTS.items(): print(n, m[3])
elif sys.argv[1] == 'apply':
    f, old, new, _ = MUTANTS[sys.argv[2]]
    path = sys.argv[3] + '/' + f
    src = open(path).read()
    if src.count(old) != 1: sys.exit('mutant %s: pattern not found once in %s' % (sys.argv[2], f))
    open(path, 'w').write(src.replace(old, new))
