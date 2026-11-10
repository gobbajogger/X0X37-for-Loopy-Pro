#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# One source mutation per LCD/SMC paint snapshot. Each must fail the host
# event-injection test. The tree is restored before exit. No device run.
set -eu
cd "$(dirname "$0")/../.."
file=firmware/src/app/smc_mixer.c
uifile=firmware/src/app/ui.c
bak=$(mktemp)
uibak=$(mktemp)
cp "$file" "$bak"
cp "$uifile" "$uibak"
restore() { cp "$bak" "$file"; cp "$uibak" "$uifile"; }
trap restore EXIT HUP INT TERM

mutate() {
    name=$1
    old=$2
    new=$3
    target=${4:-$file}
    work=$(mktemp)
    cp "$target" "$work"
    python3 - "$target" "$old" "$new" <<'PY'
import pathlib, sys
path, old, new = sys.argv[1:]
file = pathlib.Path(path)
text = file.read_text()
count = text.count(old)
if count != 1:
    raise SystemExit('anchor count %d for %s' % (count, old[:40]))
file.write_text(text.replace(old, new, 1))
PY
    set +e
    sh tests/host/smc_lcd_paint_test.sh > "$work.$name.log" 2>&1
    code=$?
    set -e
    cp "$work" "$target"
    if [ "$code" -eq 0 ]; then
        echo "MUTANT SURVIVED: $name"
        tail -20 "$work.$name.log" >&2
        exit 1
    fi
    killed=$(grep '^FAIL ' "$work.$name.log" | tr '\n' '; ' || true)
    if [ -z "$killed" ]; then
        echo "MUTANT SURVIVED: $name (no assertion failure)"
        tail -20 "$work.$name.log" >&2
        exit 1
    fi
    echo "MUTANT KILLED: $name by $killed"
}

mutate live-smc-ui \
    'static uint8_t smc_paint_kind(const struct smc_paint *p) { return p->kind; } /* PAINT_READ_KIND */
static uint8_t smc_paint_strip(const struct smc_paint *p) { return p->strip; } /* PAINT_READ_STRIP */
static uint8_t smc_paint_action(const struct smc_paint *p) { return p->action; } /* PAINT_READ_ACTION */' \
    'static uint8_t smc_paint_kind(const struct smc_paint *p) { (void)p; return smc_ui.kind; } /* PAINT_READ_KIND */
static uint8_t smc_paint_strip(const struct smc_paint *p) { (void)p; return smc_ui.strip; } /* PAINT_READ_STRIP */
static uint8_t smc_paint_action(const struct smc_paint *p) { (void)p; return smc_ui.action; } /* PAINT_READ_ACTION */'

mutate peek-skip-snapshot \
    '/* PEEK_SNAPSHOT_HOLD */' \
    'smc_paint_take(&snap); /* PEEK_SNAPSHOT_HOLD */'

mutate peek-body-live \
    '            ui.view = snap.view; /* PEEK_BODY_FROM_SNAPSHOT */
            ui.part = snap.part;
            ui.mixsel = snap.mixsel;
            memcpy(ui.sel, snap.sel, sizeof ui.sel);
            pg = pre_pg;' \
    '            /* PEEK_BODY_FROM_SNAPSHOT */
            (void)snap;
            (void)pre_pg;'

mutate drop-ui.sel-restore \
    '            memcpy(ui.sel, live.sel, sizeof ui.sel); /* LIVE_SEL_RESTORE */' \
    '            /* LIVE_SEL_RESTORE */
            (void)live;'

mutate overlay-retake-after-dma \
    '/* OVERLAY_SNAPSHOT_HOLD */' \
    'smc_paint_take(paint); /* OVERLAY_SNAPSHOT_HOLD */'

mutate grid-live-view \
    '    ui.view = paint->view; /* GRID_VIEW_FROM_SNAPSHOT */
    ui.part = paint->part;
    ui.mixsel = paint->mixsel;
    memcpy(ui.sel, paint->sel, sizeof ui.sel);' \
    '    /* GRID_VIEW_FROM_SNAPSHOT */
    (void)paint;'

mutate strip-target-live-peer \
    '        pref_t f = paint->fader[n], k = paint->knob[n]; /* GRID_TARGETS_FROM_SNAPSHOT */' \
    '        pref_t f, k; /* GRID_TARGETS_FROM_SNAPSHOT */
        {
            uint8_t live_peer = smc_ui.peer < SMC_PEERS ? smc_ui.peer : 0;
            uint8_t saved_peer = smc_slot;
            smc_slot = live_peer;
            f = smc_target(n);
            k = smc_knob_target(n);
            smc_slot = saved_peer;
        }'

mutate drop-page-index \
    '*page_of(ui.view) = peek_page; /* PEEK_PAGE_INDEX */' \
    '/* PEEK_PAGE_INDEX */'

mutate drop-swap-to-live \
    'ui_paint_around_input(); /* SWAP_TO_LIVE */' \
    '(void)0; /* SWAP_TO_LIVE */' \
    firmware/src/app/ui.c

mutate drop-swap-to-snapshot \
    'ui_paint_around_input(); /* SWAP_TO_SNAPSHOT */' \
    '(void)0; /* SWAP_TO_SNAPSHOT */' \
    firmware/src/app/ui.c

echo "smc lcd paint mutants: 10 killed"
