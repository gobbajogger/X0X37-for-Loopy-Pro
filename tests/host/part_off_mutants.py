#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# M7a mutants for tests/host/run_part_off.sh: each is applied to a copy of the tree, never to the sources.
# A mutant is killed when the case it names FAILs. usage: part_off_mutants.py list | apply <name> <root>
import sys
E_C = 'firmware/src/app/engine.c'
S_C = 'firmware/src/seq/sequencer.c'
S_H = 'firmware/src/seq/sequencer.h'
P_C = 'firmware/src/app/project.c'
X_H = 'firmware/src/app/x0x.h'
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
    # storage (a-variant): the settings object's two sizes and the tail's validation
    'store_always_long': (P_C, '    part_off &= PROJ_PART_OFF_ALL;\n    if (part_off) {',
                          '    part_off &= PROJ_PART_OFF_ALL;\n    if (1) {', 'store_all_on_is_legacy_40'),
    'store_ignore_reserved': (X_H, '#define PROJ_SET_TAIL_BAD(t) ((t)[0] > PROJ_PART_OFF_ALL || (t)[1] || (t)[2] || (t)[3])',
                              '#define PROJ_SET_TAIL_BAD(t) ((t)[0] > PROJ_PART_OFF_ALL)', 'store_rejects_reserved'),
    'store_accept_gt31': (X_H, '#define PROJ_SET_TAIL_BAD(t) ((t)[0] > PROJ_PART_OFF_ALL || (t)[1] || (t)[2] || (t)[3])',
                          '#define PROJ_SET_TAIL_BAD(t) ((t)[1] || (t)[2] || (t)[3])', 'store_rejects_mask_gt31'),
    'store_reject_40': (X_H, '#define PROJ_SET_LEN_OK(n) ((n) == 4u + sizeof(settings_t) || (n) == 4u + sizeof(settings_t) + PROJ_SET_TAIL)',
                        '#define PROJ_SET_LEN_OK(n) ((n) == 4u + sizeof(settings_t) + PROJ_SET_TAIL)', 'store_40_loads_all_on'),
    'store_drop_mask_on_load': (P_C, "uint8_t part_off = rc ? 0u : io_buf[4u + sizeof set];",
                                'uint8_t part_off = 0;', 'store_roundtrip_all_masks'),
    'store_bt_toggle_strict': (P_C, '        if (!PROJ_SET_LEN_OK((uint32_t)n) || get32(io_buf) != PROJ_FORMAT ||\n            io_buf[4u + __builtin_offsetof(settings_t, snd_rev)] != PROJ_SND_REV ||',
                               '        if ((uint32_t)n != 4u + sizeof(settings_t) || get32(io_buf) != PROJ_FORMAT ||\n            io_buf[4u + __builtin_offsetof(settings_t, snd_rev)] != PROJ_SND_REV ||',
                               'store_bt_toggle_keeps_tail'),
    # the .x0x header word 28 (part OFF mask across export/import)
    'file_header_drops_mask': (P_C, '    put32(h + 28, PROJ_FILE_OFF_WORD(part_off_staged & PROJ_PART_OFF_ALL));',
                               '    put32(h + 28, 0);', 'file_roundtrip_all_masks'),
    'file_import_drops_mask': (P_C, '    part_off_staged = (uint8_t)(get32(h + 28) & PROJ_PART_OFF_ALL);',
                               '    part_off_staged = 0;', 'file_roundtrip_all_masks'),
    'file_accepts_any_word28': (P_C, '|| !PROJ_FILE_OFF_OK(get32(h + 28)))', '|| 0)', 'file_rejects_bad_word28'),
    'file_tag_ignored': (X_H, '(((w) & 0xFFFF0000u) == PROJ_FILE_OFF_TAG &&', '(1 &&', 'file_rejects_bad_word28'),
    'file_chk_ignored': (X_H, '((w) >> 8 & 0xFFu) == PROJ_FILE_OFF_CHK((w) & 0xFFu)))', '1))', 'file_word28_bit_errors_refused'),
    'file_chk_not_written': (X_H, 'PROJ_FILE_OFF_TAG | PROJ_FILE_OFF_CHK(m) << 8 | (uint32_t)(m) : 0u)', 'PROJ_FILE_OFF_TAG | (uint32_t)(m) : 0u)', 'file_roundtrip_all_masks'),
    'file_chk_weak': (X_H, '(((m) & 16u) ? 0x67u : 0u))', '(((m) & 16u) ? 0x01u : 0u))', 'file_word28_bit_errors_refused'),
    'file_tag_mask_zero': (X_H, '((w) & 0xFFu) - 1u < PROJ_PART_OFF_ALL))', '((w) & 0xFFu) <= PROJ_PART_OFF_ALL))', 'file_rejects_bad_word28'),
    'file_accept_gt31': (X_H, '((w) & 0xFFu) - 1u < PROJ_PART_OFF_ALL))', '((w) & 0xFFu) - 1u < 0xFFu))', 'file_rejects_bad_word28'),
    'file_all_on_tagged': (X_H, '#define PROJ_FILE_OFF_WORD(m) ((m) ? PROJ_FILE_OFF_TAG |', '#define PROJ_FILE_OFF_WORD(m) ((1) ? PROJ_FILE_OFF_TAG |', 'file_all_on_is_older_header'),
    # QA X1: the staged mask set before the payload CRC check; QA X3: the header from the live mask
    'file_staged_before_crc': (P_C, '        get32(h + 24) != PROJ_SND_REV || !PROJ_FILE_OFF_OK(get32(h + 28)))\n        return -1;\n',
                               '        get32(h + 24) != PROJ_SND_REV || !PROJ_FILE_OFF_OK(get32(h + 28)))\n        return -1;\n'
                               '    part_off_staged = (uint8_t)(get32(h + 28) & PROJ_PART_OFF_ALL);\n', 'file_validate_crc_fail_keeps_staged'),
    'file_header_from_live': (P_C, 'PROJ_FILE_OFF_WORD(part_off_staged & PROJ_PART_OFF_ALL)', 'PROJ_FILE_OFF_WORD(part_off_live & PROJ_PART_OFF_ALL)',
                              'file_double_validate_keeps_imported_mask'),
    # QA D: the tail used as the mask when the load failed (fresh device), or a stale tail after 44 bytes
    # (the literal D, dropping the rc guard at read_project, is equivalent: every failed load ends at
    # failed: -> project_defaults_for, which clears the mask; this removes that guard instead)
    'store_defaults_keep_mask': (P_C, '    *part_off_of(pr) = 0;                     /* defaults: every part ON */',
                                 '    /* mutant */', 'store_fresh_device_all_on'),
    'store_stale_tail_44': (P_C, '    if ((uint32_t)n == 4u + sizeof *dst)\n        __builtin_memset(io_buf + 4u + sizeof *dst, 0, PROJ_SET_TAIL);\n    else if',
                            '    if ((uint32_t)n == 4u + sizeof *dst)\n        ;\n    else if', 'store_stale_iobuf_44_all_on'),
    # QA E: the BT save patches a long object whose tail the validator refuses
    'store_bt_save_accepts_bad_tail': (P_C, ' ||\n            ((uint32_t)n > 4u + sizeof(settings_t) && PROJ_SET_TAIL_BAD(io_buf + 4u + sizeof(settings_t))))\n            return -9;',
                                       ')\n            return -9;', 'store_bt_save_refuses_bad_tail'),
}
if sys.argv[1] == 'list':
    for n, m in MUTANTS.items(): print(n, m[3])
elif sys.argv[1] == 'apply':
    f, old, new, _ = MUTANTS[sys.argv[2]]
    path = sys.argv[3] + '/' + f
    src = open(path).read()
    if src.count(old) != 1: sys.exit('mutant %s: pattern not found once in %s' % (sys.argv[2], f))
    open(path, 'w').write(src.replace(old, new))
