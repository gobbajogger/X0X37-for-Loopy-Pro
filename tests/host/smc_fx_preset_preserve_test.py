#!/usr/bin/env python3
"""Compile the real SMC FX functions against a model of two mixer peers.

Regression that held FX SELECT publishes the chosen effect across banks and peers.
This does not build the full UI, BLE subsystem or original synthesis engines.
"""
from __future__ import annotations

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get("SMC_FX_SOURCE", str(ROOT / "firmware/src/app/smc_mixer.c")))
NAMES = (
    "smc_fx_publish", "smc_fx_load_page", "smc_fx_main_tick", "smc_fx_try",
    "smc_fx_release_note", "smc_fx_clear",
)


def production_body(text: str, name: str) -> str:
    match = re.search(r"(?m)^static (?:int|void) " + name + r"\([^;\n]*\)\s*\{", text)
    if not match:
        raise AssertionError(f"Missing real production function: {name}")
    begin = match.start()
    opening = text.index("{", match.start())
    depth = 0
    in_string = False
    escape = False
    for index in range(opening, len(text)):
        ch = text[index]
        if in_string:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[begin:index + 1]
    raise AssertionError(f"Unterminated production function: {name}")


C_HEADER = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app/smc_fx_map.h"
#include "app/smc_fx_strip.h"

enum { SMC_PEERS = 2, O_NONE = 0, V_FX = 3, B_HOME = 6 };
static struct { int view, overlay; } ui = {V_FX, O_NONE};
typedef struct {
    uint8_t home_down, fx_down, fx_owned, fx_lp_hold, fx_page, fx_ready;
    uint8_t fx_nav_armed, fx_home_used;
    uint8_t fx_effect[SMCFX_N], fx_main_wait, select_down;
    uint32_t fx_main_t0[3];
    smcfx_strip_t fx_strip;
} smc_state_t;
static smc_state_t smc_states[SMC_PEERS];
static unsigned smc_slot;
#define smc (smc_states[smc_slot])

static uint8_t published_down, published_fx[SMCFX_N];
static uint32_t now;
static uint8_t smc_fx_tap_nav_once;
static int home, clip, drum = -1, navigation, last_nav = -1, saves, resets;
static unsigned checks;
#define CHECK(pred) do { checks++; if (!(pred)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #pred); return 1; \
} } while (0)

static int smk_clip_mode_on(void) { return clip; }
static int smc_drum_mix(void) { return drum; }
static int smc_fx_bus(uint8_t note) { return note < 32u ? (note & 7u) : -1; }
static int smc_fx_bus_now(uint8_t note)
{
    return ui.view != V_FX || ui.overlay != O_NONE ||
           smk_clip_mode_on() || smc_drum_mix() >= 0 ? -1 : smc_fx_bus(note);
}
static int smc_home_held(void) { return home; }
static uint32_t plat_ms(void) { return now; }
static uint32_t plat_buttons(void) { return 0; }
static void say(const char *left, const char *right) { (void)left; (void)right; }
static void smc_fx_save_preset(void) { saves++; }
static void smc_fx_reset_cuts(void) { resets++; }
static void smc_fx_show(uint8_t note) { (void)note; }
static void smc_fx_main_nav(int which) { navigation++; last_nav = which; }
static void engine_smc_busfx(uint8_t down, const uint8_t fx[SMCFX_N])
{
    published_down = down;
    memcpy(published_fx, fx, sizeof published_fx);
}
#define SMC_FX_HOLD_MS 180u
static void smc_fx_clear(void);
static void smc_fx_publish(void);
static void smc_fx_load_page(int page);
static void smc_fx_main_tick(void);
static int smc_fx_try(uint8_t note, int mmc);
static void smc_fx_release_note(uint8_t note);
'''

C_TEST = r'''
static int all_assignments_match(const uint8_t before[SMCFX_N])
{
    return !memcmp(before, smc.fx_effect, SMCFX_N);
}
static void save_assignments(uint8_t before[SMCFX_N])
{
    memcpy(before, smc.fx_effect, SMCFX_N);
}
static int valid_assignments(void)
{
    for (int bus = 0; bus < SMCFX_N; bus++)
        if (!smcfx_suit_ok(bus, smc.fx_effect[bus])) return 0;
    return 1;
}

int main(void)
{
    uint8_t before[SMCFX_N];
    for (int pg = 0; pg < SMCFX_NPAGE; pg++) {
        for (int bus = 0; bus < SMCFX_N; bus++) {
            memset(smc_states, 0, sizeof smc_states);
            smc_slot = 0;
            home = clip = navigation = 0;
            drum = -1;
            ui.view = V_FX; ui.overlay = O_NONE;
            published_down = 0; now = 10;
            smc_fx_load_page(pg);
            CHECK(valid_assignments());
            save_assignments(before);
            const uint8_t note = SMCFX_NOTE[bus];
            const uint8_t bit = (uint8_t)(1u << bus);
            const int page_select_empty =
                smcfx_strip_get(&smc.fx_strip, pg, bus, SMCFX_SELECT) == SMCFX_EMPTY;
            if (smcfx_is_main(bus)) smc.select_down |= bit;
            CHECK(smc_fx_try(note, 0) == 1);
            CHECK(all_assignments_match(before)); /* an ordinary tap cannot rewrite the preset */
            if (smcfx_is_main(bus)) {
                CHECK(!(published_down & bit)); /* navigation pending, no FX yet */
                now += SMC_FX_HOLD_MS - 1u;
                smc_fx_main_tick();
                CHECK(!(published_down & bit));
                int old_navigation = navigation;
                smc_fx_release_note(note);
                CHECK(navigation == old_navigation + 1);
                CHECK(last_nav == bus - SMCFX_RVB);
                smc.select_down &= (uint8_t)~bit;
            } else {
                CHECK(!!(published_down & bit) == !page_select_empty);
                if (!page_select_empty) CHECK(published_fx[bus] == before[bus]);
                smc_fx_release_note(note);
            }
            CHECK(!(published_down & bit));
            CHECK(all_assignments_match(before));
            CHECK(valid_assignments());
            CHECK(!(smc.fx_lp_hold & bit));
            /* Cycling must use the original selected effect, not a stale LPF. */
            home = 1;
            CHECK(smc_fx_try(note, 0) == 1);
            CHECK(smc.fx_effect[bus] == smcfx_cycle(bus, before[bus]));
            CHECK(smcfx_suit_ok(bus, smc.fx_effect[bus]));
            const uint8_t chosen = smc.fx_effect[bus];
            home = 0;
            /* Play a re-assigned bus FX, not the stale low-pass hold flag. */
            if (smcfx_is_main(bus)) smc.select_down |= bit;
            CHECK(smc_fx_try(note, 0) == 1);
            if (smcfx_is_main(bus)) {
                CHECK(!(published_down & bit));
                now += SMC_FX_HOLD_MS + 1u;
                smc_fx_main_tick();
            }
            CHECK(!!(published_down & bit));
            CHECK(published_fx[bus] == chosen);
            smc_fx_release_note(note);
            CHECK(!(published_down & bit));
            CHECK(smc.fx_effect[bus] == chosen);
            if (smcfx_is_main(bus)) smc.select_down &= (uint8_t)~bit;
        }
    }

    /* All four row buttons on both pages are momentary, individually suited,
     * and the unassigned page-two buttons remain silent. */
    for (int pg = 0; pg < SMCFX_NPAGE; pg++)
        for (int bus = 0; bus < SMCFX_N; bus++)
            for (int button = SMCFX_REC; button <= SMCFX_MUTE; button++) {
                memset(smc_states, 0, sizeof smc_states);
                smc_slot = 0; home = clip = 0; drum = -1;
                ui.view = V_FX; ui.overlay = O_NONE;
                smc_fx_load_page(pg);
                uint8_t fx = smcfx_strip_get(&smc.fx_strip, pg, bus, button);
                uint8_t note = (uint8_t)(button * 8 + bus);
                uint8_t bit = (uint8_t)(1u << bus);
                CHECK(smc_fx_try(note, 0) == 1);
                CHECK(!!(published_down & bit) == (fx != SMCFX_EMPTY));
                if (fx != SMCFX_EMPTY) CHECK(published_fx[bus] == fx);
                smc_fx_release_note(note);
                CHECK(!(published_down & bit));
            }
    /* Two simultaneously held buttons on one bus return to the earlier
     * effect when the later button is released. */
    memset(smc_states, 0, sizeof smc_states);
    smc_slot = 0; home = 0; ui.view = V_FX; ui.overlay = O_NONE;
    smc_fx_load_page(0);
    CHECK(smc_fx_try(2, 0) == 1); /* 303A REC -> HPF */
    CHECK(published_fx[SMCFX_303A] == SMCFX_HP);
    CHECK(smc_fx_try(26, 0) == 1); /* 303A SELECT -> LPF */
    CHECK(published_fx[SMCFX_303A] == SMCFX_LP);
    smc_fx_release_note(26);
    CHECK(published_fx[SMCFX_303A] == SMCFX_HP);
    smc_fx_release_note(2);
    CHECK(!(published_down & (1u << SMCFX_303A)));

    /* Long main-bus hold must publish the chosen effect, not forced LPF. */
    for (int bus = SMCFX_RVB; bus < SMCFX_N; bus++) {
        memset(smc_states, 0, sizeof smc_states);
        smc_slot = 0; now = 30; navigation = home = 0;
        ui.view = V_FX; ui.overlay = O_NONE;
        smc_fx_load_page(0);
        save_assignments(before);
        uint8_t bit = (uint8_t)(1u << bus);
        smc.select_down |= bit;
        CHECK(smc_fx_try(SMCFX_NOTE[bus], 0) == 1);
        now += SMC_FX_HOLD_MS + 1u;
        smc_fx_main_tick();
        CHECK(published_down & bit);
        CHECK(published_fx[bus] == before[bus]);
        CHECK(all_assignments_match(before));
        smc_fx_release_note(SMCFX_NOTE[bus]);
        CHECK(!navigation);
        CHECK(!(published_down & bit));
        CHECK(all_assignments_match(before));
        CHECK(!(smc.fx_lp_hold & bit));
        smc.select_down &= (uint8_t)~bit;
    }

    /* A lost main-bus key-up must publish an immediate FX-off state. */
    for (int bus = SMCFX_RVB; bus < SMCFX_N; bus++) {
        memset(smc_states, 0, sizeof smc_states);
        smc_slot = 0; now = 1000; navigation = home = 0;
        ui.view = V_FX; ui.overlay = O_NONE;
        smc_fx_load_page(0);
        save_assignments(before);
        uint8_t bit = (uint8_t)(1u << bus);
        smc.select_down |= bit;
        CHECK(smc_fx_try(SMCFX_NOTE[bus], 0) == 1);
        now += SMC_FX_HOLD_MS + 1u;
        smc_fx_main_tick();
        CHECK(published_down & bit);
        smc.select_down &= (uint8_t)~bit; /* lost release event */
        smc_fx_main_tick();
        CHECK(!(published_down & bit));
        CHECK(!(smc.fx_down & bit));
        CHECK(!(smc.fx_lp_hold & bit));
        CHECK(!(smc.fx_owned & bit));
        CHECK(all_assignments_match(before));
    }

    /* MMC toggles the chosen effect, without changing the stored bank. */
    memset(smc_states, 0, sizeof smc_states);
    smc_slot = 0; smc_fx_load_page(0); save_assignments(before);
    CHECK(smc_fx_try(24, 1) == 1);
    CHECK(published_down & 1u);
    CHECK(published_fx[0] == before[0]);
    CHECK(all_assignments_match(before));
    CHECK(smc_fx_try(24, 1) == 1);
    CHECK(!(published_down & 1u));
    CHECK(!(smc.fx_lp_hold & 1u));
    CHECK(all_assignments_match(before));

    /* A captured press cannot leave a stale held bit after clear. */
    CHECK(smc_fx_try(24, 0) == 1);
    smc_fx_clear();
    CHECK(!smc.fx_lp_hold && !smc.fx_down && !smc.fx_owned);
    CHECK(all_assignments_match(before));

    /* Two peers retain their independent presets under the shared FX bus. */
    memset(smc_states, 0, sizeof smc_states);
    smc_slot = 0; smc_fx_load_page(0);
    smc_slot = 1; smc_fx_load_page(1);
    home = 1; CHECK(smc_fx_try(24, 0) == 1); /* fill empty SELECT on page 2 */
    home = 0;
    uint8_t peer_one_fx = smc_states[1].fx_effect[0];
    smc_slot = 1; CHECK(smc_fx_try(24, 0) == 1);
    CHECK(published_fx[0] == peer_one_fx);
    smc_slot = 0; CHECK(smc_fx_try(24, 0) == 1);
    CHECK(published_fx[0] == SMCFX_PAGE[0][0]); /* peer 0 wins */
    smc_fx_release_note(24);
    CHECK(published_down & 1u); /* peer 1 is still held */
    CHECK(published_fx[0] == peer_one_fx);
    smc_slot = 1; smc_fx_release_note(24);
    CHECK(!(published_down & 1u));
    CHECK(smc_states[0].fx_effect[0] == SMCFX_PAGE[0][0]);
    CHECK(smc_states[1].fx_effect[0] == peer_one_fx);

    /* Non-FX context leaves assignments untouched and rejects the gesture. */
    smc_slot = 0; ui.view = 0; save_assignments(before);
    CHECK(smc_fx_try(24, 0) == 0);
    CHECK(all_assignments_match(before));
    printf("PASS SMC FX preset preservation: %u assertions\n", checks);
    return 0;
}
'''


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    function_sources = [production_body(source, name) for name in NAMES]
    code = C_HEADER + "\n\n".join(function_sources) + "\n" + C_TEST
    compilers = os.environ.get("SMC_FX_VERIFY_CC", os.environ.get("CC", "cc")).split()
    if not compilers:
        raise RuntimeError("No C compiler selected")
    with tempfile.TemporaryDirectory(prefix="smc-fx-presets-") as tmp:
        tmpdir = Path(tmp)
        cfile = tmpdir / "production.c"
        exe = tmpdir / "production"
        cfile.write_text(code)
        args = compilers + ["-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-Ifirmware/src", str(cfile), "firmware/src/app/smc_fx_map_tables.c", "-o", str(exe)]
        if os.environ.get("SAN_FLAGS"):
            args[1:1] = os.environ["SAN_FLAGS"].split()
        subprocess.run(args, cwd=ROOT, check=True)
        subprocess.run([str(exe)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()