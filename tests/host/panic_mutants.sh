#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# One source mutation per new panic behaviour. Each mutation must fail its host test.
# Device and native runs are not this script. The tree is restored before exit.
set -eu
cd "$(dirname "$0")/../.."
bak=$(mktemp -d "${TMPDIR:-/tmp}/x0x-panic-mut.XXXXXX")
files="firmware/src/seq/sequencer.c firmware/src/app/engine.c firmware/src/app/ui.c firmware/src/app/smc_mixer.c"
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
    if [ "$code" -ne 1 ] || ! grep -Eq '^FAIL tests/host/panic_(notes|clear|route)_test\.c:[0-9]+:' "$bak/$name.log"; then
        echo "MUTANT INVALID: $name (test did not reach a behavior assertion)" >&2
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
        if (panic_cycle && (type == C_DRUM || type == C_BON || type == C_REC ||' \
    '        /* PANIC_DROP_QUEUED */
        if (0 && (type == C_DRUM || type == C_BON || type == C_REC ||' \
    tests/host/panic_clear_test.sh

mutate silence-skipped firmware/src/app/engine.c \
    '    if (panic_cycle) {
        /* Physical Free Play has its own FIFO, drained later in this block.
         * Consume those pending presses before silencing every voice. */
        live_key_drain();
        engine_notes_off_apply();' \
    '    if (0) {
        /* Physical Free Play has its own FIFO, drained later in this block.
         * Consume those pending presses before silencing every voice. */
        live_key_drain();
        engine_notes_off_apply();' \
    tests/host/panic_clear_test.sh

mutate physical-note-kept firmware/src/app/engine.c \
    '        live_key_drain();
        engine_notes_off_apply();' \
    '        engine_notes_off_apply();' \
    tests/host/panic_clear_test.sh

mutate peer-state-shared firmware/src/app/smc_mixer.c \
    '#define smc_panic_peer (smc_panic_peers[smc_slot])' \
    '#define smc_panic_peer (smc_panic_peers[0])' \
    tests/host/panic_route_test.sh

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

mutate bank-release-stale firmware/src/app/smc_mixer.c \
    '        memset(smc.held, 0, sizeof smc.held);
#ifdef SMC_PANIC_HOOKS
        smc_panic_reset();' \
    '        memset(smc.held, 0, sizeof smc.held);
#ifdef SMC_PANIC_HOOKS
        (void)0;' \
    tests/host/panic_route_test.sh

mutate break-phase-reset firmware/src/app/engine.c \
    '    brk.running = brk_running;' \
    '    (void)brk_running;' \
    tests/host/panic_clear_test.sh

mutate drum808-not-silenced firmware/src/app/engine.c \
    '    drum808_all_off(&d808);' \
    '    (void)d808;' \
    tests/host/panic_clear_test.sh

echo "panic mutants: 11 killed"
