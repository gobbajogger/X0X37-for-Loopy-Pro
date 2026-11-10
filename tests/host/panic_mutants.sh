#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# One source mutation per new panic behaviour. Each mutation must fail its host test.
# Device and native runs are not this script. The tree is restored before exit.
set -eu
cd "$(dirname "$0")/../.."
bak=$(mktemp -d "${TMPDIR:-/tmp}/x0x-panic-mut.XXXXXX")
files="firmware/src/seq/sequencer.c firmware/src/app/engine.c firmware/src/app/ui.c firmware/src/app/smc_mixer.c firmware/src/usb.c host/x0x_host.c"
restore() {
    for f in $files; do
        cp "$bak/$(basename "$f")" "$f"
    done
}
for f in $files; do
    cp "$f" "$bak/$(basename "$f")"
done
trap restore EXIT HUP INT TERM

mutate() {
    name=$1
    file=$2
    old=$3
    new=$4
    test_sh=$5
    python3 - "$file" "$old" "$new" <<'PY'
import pathlib, sys
path, old, new = sys.argv[1:]
file = pathlib.Path(path)
text = file.read_text()
count = text.count(old)
if count != 1:
    raise SystemExit('anchor count %d' % count)
file.write_text(text.replace(old, new, 1))
PY
    set +e
    sh "$test_sh" > "$bak/$name.log" 2>&1
    code=$?
    set -e
    restore
    if [ "$code" -eq 0 ]; then
        echo "MUTANT SURVIVED: $name"
        tail -30 "$bak/$name.log" >&2
        exit 1
    fi
    echo "MUTANT KILLED: $name"
}

mutate notes-off-empty firmware/src/seq/sequencer.c \
    '    /* PANIC_NOTE_OFF */
    bass_and_brk_stop(s, o);' \
    '    /* PANIC_NOTE_OFF */
    (void)s; (void)o;' \
    tests/host/panic_notes_test.sh

mutate transport-stops firmware/src/seq/sequencer.c \
    '    /* PANIC_NOTE_OFF */
    bass_and_brk_stop(s, o);' \
    '    /* PANIC_NOTE_OFF */
    s->playing = 0;
    bass_and_brk_stop(s, o);' \
    tests/host/panic_notes_test.sh

mutate queued-note-kept firmware/src/app/engine.c \
    '        /* PANIC_DROP_QUEUED */
        if (discard && (type == C_DRUM || type == C_BON || type == C_REC ||' \
    '        /* PANIC_DROP_QUEUED */
        if (0 && (type == C_DRUM || type == C_BON || type == C_REC ||' \
    tests/host/panic_clear_test.sh

mutate silence-skipped firmware/src/app/engine.c \
    '    if (panic_cycle)
        engine_notes_off_apply();' \
    '    if (0)
        engine_notes_off_apply();' \
    tests/host/panic_clear_test.sh

mutate chord-not-consumed firmware/src/app/ui.c \
    '        /* PANIC_CONSUME */
        if ((i == B_FX || i == B_SEQ) && panic_consume) {' \
    '        /* PANIC_CONSUME */
        if ((i == B_FX || i == B_SEQ) && 0) {' \
    tests/host/panic_route_test.sh

mutate user-mode-fires firmware/src/app/smc_mixer.c \
    '    /* PANIC_DAW_ONLY */
    mode = (proj.set.bt & PROJ_BT_SMC_USER) ? X0X_PANIC_SMC_USER : X0X_PANIC_SMC_DAW;' \
    '    /* PANIC_DAW_ONLY */
    mode = X0X_PANIC_SMC_DAW;' \
    tests/host/panic_route_test.sh

mutate overflow-keeps-slot firmware/src/app/engine.c \
    '                cq_r = r + 1u;
                i = w;
                cq_w = w + 1u;' \
    '                i = r;' \
    tests/host/panic_order_test.sh

mutate midi-defer-ignored firmware/src/app/engine.c \
    '            (int16_t)((uint16_t)seq - (uint16_t)notes_off_midi_head) >= 64)' \
    '            0 && (int16_t)((uint16_t)seq - (uint16_t)notes_off_midi_head) >= 64)' \
    tests/host/panic_order_test.sh

mutate seq-carry-dropped firmware/src/usb.c \
    '                /* PANIC_SEQ_CARRY */
                if (seq)
                    seq[i % MQ] = seq[(i + 1u) % MQ];' \
    '                /* PANIC_SEQ_CARRY */' \
    tests/host/midi_seq_carry_test.sh

mutate cq-unsigned-order firmware/src/app/engine.c \
    '        pre_cmd = notes_off_drop && notes_off_cq_live && (int32_t)(ord - notes_off_cq) < 0;' \
    '        pre_cmd = notes_off_drop && notes_off_cq_live && ord < notes_off_cq;' \
    tests/host/panic_order_test.sh

mutate midi-unsigned-order firmware/src/app/engine.c \
    '            (int16_t)((uint16_t)seq - (uint16_t)notes_off_midi_limit) < 0;' \
    '            seq < notes_off_midi_limit;' \
    tests/host/panic_order_test.sh

mutate cq-no-reclaim firmware/src/app/engine.c \
    '    (*cursor)++;' \
    '    /* cursor stays: consumed slots do not free capacity */' \
    tests/host/cq_reclaim_test.sh

mutate shift-outside-lock firmware/src/app/engine.c \
    '                /* SHIFT */
                cq[i % CQ] = cq[src % CQ];
                cq_hole_at = src;
                cq_hole_on = 1;' \
    '                /* SHIFT */
                plat_cq_unlock(state);
                cq[i % CQ] = cq[src % CQ];
                cq_hole_at = src;
                cq_hole_on = 1;
                state = plat_cq_lock();' \
    tests/host/cq_shift_preempt_test.sh

mutate no-shift firmware/src/app/engine.c \
    '                /* SHIFT */
                cq[i % CQ] = cq[src % CQ];
                cq_hole_at = src;
                cq_hole_on = 1;
                i = src;' \
    '                /* SHIFT */
                publish = 1;' \
    tests/host/panic_order_test.sh

mutate cq-tail-boundary firmware/src/app/engine.c \
    '            } else if (i + 1u == w) {
                if (notes_off_cq_live && (int32_t)(i - notes_off_cq) < 0)
                    notes_off_cq--;
                publish = 1;' \
    '            } else if (i + 1u == w) {
                if (notes_off_cq_live && (int32_t)(i - notes_off_cq) < 0)
                    ;
                publish = 1;' \
    tests/host/panic_order_test.sh

mutate cq-shift-boundary firmware/src/app/engine.c \
    '                if (notes_off_cq_live && (src == notes_off_cq || src + 1u == notes_off_cq))
                    notes_off_cq--;' \
    '                if (notes_off_cq_live && (src == notes_off_cq || src + 1u == notes_off_cq))
                    ;' \
    tests/host/panic_order_test.sh

mutate cq-pick-hole firmware/src/app/engine.c \
    '    if (cq_hole_on && *cursor == cq_hole_at)
        *cursor = *cursor + 1u;' \
    '    /* hole slot is returned */' \
    tests/host/cq_shift_preempt_test.sh

mutate live-empty-rebase firmware/src/app/engine.c \
    '    if (live_key_r == live_key_w)
        live_key_at = live_key_pos;
    live_key_pos++;' \
    '    live_key_pos++;' \
    tests/host/panic_order_test.sh

mutate live-unsigned-drop firmware/src/app/engine.c \
    '        if(!post && notes_off_drop && (int32_t)(ord - notes_off_live) < 0)continue;' \
    '        if(!post && notes_off_drop && ord < notes_off_live)continue;' \
    tests/host/live_key_wrap_test.sh

mutate live-unsigned-post firmware/src/app/engine.c \
    '        int post = live_panic_pass && (int32_t)(ord - notes_off_live) >= 0;' \
    '        int post = live_panic_pass && ord >= notes_off_live;' \
    tests/host/live_key_wrap_test.sh

mutate cq-head-no-advance firmware/src/app/engine.c \
    '            if (i == r) {
                cq_r = r + 1u;
                i = w;
                cq_w = w + 1u;' \
    '            if (i == r) {
                i = w;
                cq_w = w + 1u;' \
    tests/host/cq_reclaim_test.sh

mutate cq-inplace-replace firmware/src/app/engine.c \
    '                cq_r = r + 1u;
                i = w;
                cq_w = w + 1u;' \
    '                i = r;' \
    tests/host/cq_reclaim_test.sh

mutate host-midi-skip0 host/x0x_host.c \
    '    mi_seq_next = n ? n : 1u;' \
    '    mi_seq_next = n;' \
    tests/host/midi_seq_skip0_test.sh

mutate usb-midi-skip0 firmware/src/usb.c \
    '    midi_seq_next = n ? n : 1u;' \
    '    midi_seq_next = n;' \
    tests/host/midi_seq_take_test.sh

mutate host-midi-plus2 host/x0x_host.c \
    '    uint16_t n = (uint16_t)(s + 1u);' \
    '    uint16_t n = (uint16_t)(s + 2u);' \
    tests/host/midi_seq_skip0_test.sh

mutate usb-midi-plus2 firmware/src/usb.c \
    '    uint16_t n = (uint16_t)(s + 1u);' \
    '    uint16_t n = (uint16_t)(s + 2u);' \
    tests/host/midi_seq_take_test.sh

echo "panic mutants: 26 killed"
