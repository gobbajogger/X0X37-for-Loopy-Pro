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

mutate drop-cmp-page \
    's->page = (uint8_t)page; /* CMP_PAGE */' \
    '/* CMP_PAGE */'

mutate drop-cmp-select \
    's->mixsel = ui.mixsel; /* CMP_SELECT */' \
    '/* CMP_SELECT */'

mutate drop-cmp-param \
    '    for (i = 0; i < 8; i++) {
        s->param[i] = smc_scaled(paint->fader[i]);
        s->param[8 + i] = smc_scaled(paint->knob[i]);
    } /* CMP_PARAM */' \
    '    /* CMP_PARAM */'

mutate drop-cmp-rec \
    '        s->led[i] = (uint8_t)(smc_led(i) ? 1 : 0); /* CMP_REC */' \
    '        s->led[i] = 0; /* CMP_REC */'

mutate drop-cmp-play \
    '    s->playing = (uint8_t)(seq.playing != 0);
    s->bpm_l = (uint8_t)bpm; /* CMP_PLAY */
    s->bpm_h = (uint8_t)(bpm >> 8); /* CMP_BPMH */' \
    '    /* CMP_PLAY */'

mutate drop-cmp-bpmh \
    '    s->bpm_h = (uint8_t)(bpm >> 8); /* CMP_BPMH */' \
    '    /* CMP_BPMH */'

mutate drop-cmp-overlay \
    '    s->overlay = ui.overlay;
    memcpy(s->list, list_title, sizeof s->list); /* CMP_OVERLAY */' \
    '    /* CMP_OVERLAY */'

mutate drop-cmp-lock \
    '    s->locked = smc.locked;
    s->lock_view = smc.lock_view;
    s->lock_page = smc.lock_page; /* CMP_LOCKPAGE */
    s->lock_bank = smc.lock_bank;
    s->lock_part = smc.lock_part; /* CMP_LOCK */' \
    '    /* CMP_LOCK */'

mutate drop-cmp-lockpage \
    '    s->lock_page = smc.lock_page; /* CMP_LOCKPAGE */' \
    '    /* CMP_LOCKPAGE */'

mutate drop-cmp-pickup \
    '    s->picked = smc.picked;
    memcpy(s->last, smc.last, sizeof s->last); /* CMP_PICKUP */' \
    '    /* CMP_PICKUP */'

mutate drop-cmp-fx \
    '    s->fx_page = smc.fx_page; /* CMP_FXPAGE */
    s->fx_down = smc.fx_down;
    s->fx_lp_hold = smc.fx_lp_hold; /* CMP_FXLP */
    memcpy(s->fx_effect, smc.fx_effect, sizeof s->fx_effect); /* CMP_FX */' \
    '    /* CMP_FX */'

mutate drop-cmp-fxpage \
    '    s->fx_page = smc.fx_page; /* CMP_FXPAGE */' \
    '    /* CMP_FXPAGE */'

mutate drop-cmp-fxlp \
    '    s->fx_lp_hold = smc.fx_lp_hold; /* CMP_FXLP */' \
    '    /* CMP_FXLP */'

mutate drop-cmp-rec-byte \
    '    s->rec = ui.rec; /* CMP_REC_BYTE */' \
    '    /* CMP_REC_BYTE */'

mutate flip-body-guard \
    '            draw_main();
            draw_knobs();' \
    '            smc_states[0].held[0] ^= 1u;
            smc_panic_peers[0].held ^= 1u;
            draw_main();
            draw_knobs();
            smc_states[0].held[0] ^= 1u;
            smc_panic_peers[0].held ^= 1u;'

mutate drop-cmp-bank \
    's->knob_bank = smc.knob_bank; /* CMP_BANK */' \
    '/* CMP_BANK */'

mutate drop-cmp-voice \
    '        const char *name = engine_voice_name(kit ? T_808 : T_909, ui.sel[kit]);
        int n = 0;
        s->sel0 = ui.sel[kit];
        while (name && name[n] && n < 11) {
            s->voice[n] = name[n];
            n++;
        } /* CMP_VOICECHARS */' \
    '        (void)kit; /* CMP_VOICE */'

mutate drop-cmp-voicechars \
    '        while (name && name[n] && n < 11) {
            s->voice[n] = name[n];
            n++;
        } /* CMP_VOICECHARS */' \
    '        (void)name; (void)n; /* CMP_VOICECHARS */'

mutate swap-memcmp \
    'cmp = memcmp(&now, base, sizeof now); /* CMP_ORDER */' \
    'cmp = memcmp(base, &now, sizeof now); /* CMP_ORDER */'

mutate drop-cmp-listtop \
    '    s->list_top = ui.list_top; /* CMP_LISTTOP */' \
    '    /* CMP_LISTTOP */'

mutate drop-cmp-clip \
    '    s->clip = (uint8_t)(smk_clip_mode_on() ? 1 : 0); /* CMP_CLIP */' \
    '    /* CMP_CLIP */'

mutate drop-cmp-lockdrum \
    '    memcpy(s->drum_solo, smc.drum_solo, sizeof s->drum_solo);
    memcpy(s->vmute, &seq.vmute, sizeof s->vmute); /* CMP_LOCKDRUM */' \
    '    /* CMP_LOCKDRUM */'

mutate drop-cmp-kind3 \
    '        s->kind3 = 1;
        s->view = ui.view;
        s->fx_bus = (uint8_t)(bus < 0 ? 0 : bus + 1);' \
    '        (void)bus; /* CMP_KIND3 */'

mutate drop-cmp-repaint \
    'smc_paint_take(paint); /* CMP_REPAINT */' \
    '(void)paint; /* CMP_REPAINT */'

mutate drop-cmp-dirty \
    'ui_lcd_mark_dirty(); /* CMP_DIRTY */' \
    '(void)0; /* CMP_DIRTY */'

echo "smc lcd paint mutants: 35 killed"
