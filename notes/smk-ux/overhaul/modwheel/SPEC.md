# Mod-wheel on-screen feedback — placement spec for Firmware Main Builder

**Status:** design proposal. Not a firmware patch. FMB is the single `ui.c` writer.
**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.
**Trace:** `/workspace/modwheel-trace/TRACE.md` (sha `1e35ae21b4f65d692ddad774200bf9d3545c2642db5504bd08d147a25e8288f4`), verified against `/workspace/smk-ux-src` at the same commit. Corrections are in §1. Claims not repeated as facts until they passed that check.
**Direction:** consistent with the overhaul recommendation in `/workspace/smk-ux/overhaul/design.md`: **R’s SOUND and first play, L’s unsaved-never-timeout, S’s pitched SEQ roll**. Shipped glyphs and packed felucca. No new page, no page change mid-performance, no work on the note-scan path, no draw from an ISR.
**Owner (relayed):** the wheel shows like **knob feedback**, with the **actual routed parameter and value**, or **multiple targets**, or an honest **unassigned**. Never a raw `CC1` label. Never interrupt notes.

Chicago is the 303 engine. Miami is never a 303 and is never labelled `303`, `303A`, or `303B`. Slot letter A/B is the part, not the engine.

---

## 1. Trace audit (what was verified, what was corrected)

Verified at `94197fc8` in `/workspace/smk-ux-src`, line numbers as in the trace unless noted.

| Claim | Verdict | Evidence |
|---|---|---|
| Physical wheel latches the selected bass part at gesture start, any page | **Confirmed** | `smk_wheels` `main_fm1.c` 1088–1099: `if (cut_part < 0 && mw && is_303()) cut_part = bidx();` then `engine_bass_mod`. `is_303()` is `PART_303A` or `PART_303B` (`ui.c` 101, 342), with no view test. Latch held across page/part changes until `mw == 0` (1097–1098, 1061–1063). |
| 909 / 808 / BREAK / FX / master have no mod-wheel target | **Confirmed** | If the gesture starts on a non-303, `cut_part` stays `-1` and there is no write. Pitch wheel *does* bend BREAK (`1067–1071`); the mod wheel does not. |
| MIDI CC1 hits **every** bass part on that channel (up to two targets) | **Confirmed** | `engine.c` 1744–1745, 1778–1781: `for (k = 0; k < NBASS; k++) if (seq.ch[TRK_BASS0+k] == ch)` and CC1 (`ty == 0xb0 && d1 == 1`) calls `mod_apply` without stopping at the first match. Default channels `SEQ_CH_DEFAULT = {9,10,1,2,3}` (`sequencer.c` 34): 909=10, 808=11, 303A=2, 303B=3, BREAK=4 (1-based). `SEQ_CH_OFF` is `0xFF` (`sequencer.h` 53) and will not match 0..15. |
| Targets: Chicago CUTOFF, Miami WOBBLE (depth), Juno CUTOFF, Drone BRIGHTNESS | **Confirmed** | `mod_param` `engine.c` 588–590: Juno → 2, Drone → 1, Miami → `ORIGINAL_MIAMI_WOBBLE_DEPTH` (12), else `ORIGINAL_CHICAGO_CUTOFF` (2). UI names in `ui_original_controls.h` 36–51: Chicago `[2]="CUTOFF"`, Miami `[12]="WOBBLE"`, Juno `[2]="CUTOFF"`, Drone `[1]="BRIGHTNESS"`. The comment at `main_fm1.c` 988–990 is **stale** (names only Chicago and Miami); `mod_param` is the label source, not that comment. |
| Wheel **replaces** the live value; rest writes live **0**, not the saved pot | **Confirmed** | `mod_apply` 1236–1242: clamp 0..127, `apply_param` (runtime only, no `pref_set`, `engine.c` 663–682), then `mod_ctl[v] = val`. Physical rest: `want = mw` with `mw == 0`, then `cut_part = -1`. `mod_ctl` stays **0**, not `-1`. `-1` is only when a later knob edit of the same param clears it (`apply_param` 672–673). Pitch wheel, by contrast, calls `engine_set_base` on release (`1079–1080`). |
| Knob feedback 900 ms, 54×55 cells, `draw_readout` 224×50, FONT_B name, FONT_M value | **Confirmed, with a placement precision** | `ui.touch_until = plat_ms() + 900` (`ui.c` 2986, 4674). Cleared in `draw_main` 6816–6820. **54×55 is the knob-strip cell**, `night_panel(i*60+3, 13, 54, 55, …)` in `draw_knobs` at **8353**, not the SOUND 4-row night matrix. `draw_readout` 6455–6504: `bx=8, by=14, bw=224, bh=50`, FONT_B name, FONT_M value, FONT_S unit. Drawn on **band 1** (`6892–6894`) only when **not** (`V_PART` and drum or 303) — on a 303 SOUND page the in-place knob cell is used instead of the box. Screen position of the box: band 1 starts at `MAIN_Y+BAND_H = 89` (`4693–4694`), so the box is at **(8, 103), 224×50**, ending at y=153, just above `KNOB_Y=158`. |
| Faders use `say()` for 1300 ms | **Confirmed** | `say` 402–408: `ui.msg_until = plat_ms() + 1300`. Faders `main_fm1.c` 960–978. `draw_header` 5345–5366: a non-`PICK` message **replaces the entire header** (fills `240×(HDR_H-1)` with `part_col()`, FONT_B or FONT_XS if wider than 232). |
| `matrix_value` / `pref_get` show the saved pot, not `mod_ctl` | **Confirmed** | `pref_get` 632: `proj.sound.v[r.a][r.b][r.c]`. `original_display_value` 2886–2891: `pref_get`, then p-lock / motion, **never** `mod_ctl`. `matrix_value` 7040–7050: the same. `pref_value` for T_303 goes through that path (1282–1285). Reusing the cell or `draw_readout` without a live read would show the pot while the filter sits at 0. |
| There is no wheel UI today | **Confirmed** | `smk_wheels` never calls `say()` and never sets `ui.touched`. |
| Display units are “0..127 for Chicago / Miami, often % for Juno / Drone” | **Corrected** | **All four targets already display as percent** in the knob readout. `original_value_text` `ui.c` **1075**: Juno and Drone `put_i(num,(value*100+63)/127); put_s(unit,"%")`. Chicago CUTOFF (id 2) and Miami WOBBLE (id 12) are not special cases and **fall through to the same formula at 1110**. Recommend that law, not a raw 0..127, so the wheel matches the knob. |
| FMB example label `303A WOBBLE` | **Must not be used** | Miami’s engine name is `MIAMI` (`BENGINE_N` `ui.c` 426, and the header at 5330–5332 already writes `MIAMI A` / `MIAMI B`). A `303A` prefix on a Miami wobble is the overhaul’s forbidden 303B-style lie. |
| Unassigned CC1 on a 909 channel can be toasted from the existing hook | **Corrected** | Physical unassigned is visible in `smk_wheels` (`mw` changes, `cut_part` stays `-1`). Incoming CC1 that matches **no** bass part never sets `ext_touch` (`mod_apply` is not called). The main-loop hook cannot see it. Do **not** add detection in `midi_in` (audio ISR, voice path). Unassigned is required for the **physical** wheel; silent unused CC1 stays silent. |

`say()` itself is a short copy, safe on the main loop, and must not be called from `midi_in` or `smk__wheel`. Drawing is only in `ui_frame` (≤ 60 Hz). All of that is confirmed (`ui.c` 8821, `main_fm1.c` 2565–2583).

---

## 2. Decision (what to ship)

**Do not use `say()` as the wheel’s surface.** It replaces the whole header (`ui.c` 5345–5366) and would hide L’s `UNSAVED`, the play icon, the pattern badge, and `Q:` (pattern cue). The owner asked for **knob** feedback, which is 900 ms and a 224×50 `draw_readout` box (or an in-place 54×55 knob cell), not a 1300 ms header wipe.

**Phase 1 (meets the owner; recommended first land):** reuse the existing `draw_readout` geometry — 224×50 at band-1 `(8, 14)` relative, screen **(8, 103)** — with the **live** `mod_ctl` value, the **routed** name, 900 ms from the last accepted move, no page change. Cover single, two targets, and physical unassigned. When live ≠ patch, a dim `PATCH  nn %` line in the same box (smallest honest rest-at-0 cue). FMB’s `say()` path stays a measured alternative only if Phase 1 cannot link; it is not the design intent.

**Phase 2:** `matrix_value` / the knob-strip 54×55 cell also read `mod_ctl` when it is `>= 0` for that part’s mod param, so after rest the cell agrees with the engine (live 0) instead of the pot. A small `LIVE` mark on that cell while `mod_ctl >= 0`. No musical change: do not restore the pot on release.

---

## 3. Placement

The feedback is an overlay on the **current** page. SEQ, SOUND, MIX, FX, REC, HOME, PERF, GLOBAL, clip-off FREE PLAY: the box is the same. Do not jump to the cutoff page. Do not open a new view. Do not draw on the clip “WAITING FOR LOOPY PRO” path (follow the existing `draw_main` exclusions at 6844–6863).

```
y=0        header 20 px     — untouched (L rail, UNSAVED, play, P1, Q: stay)
y=20..88   band 0
y=89..157  band 1           — 224×50 box at (8, 103)..(231, 152), part-colour frame, C_BLACK fill
y=158..239 knob strip       — Phase 2 only: if the target is one of the four 54×55 cells, also show the live number there
```

This is the shipped `draw_readout` site (`ui.c` 6487–6503). On a 303 SOUND page, shipped knobs skip this box and only highlight the cell (`6892–6894`). For the **wheel**, always use the box: the target is often not on the current knob bank (Juno CUTOFF is bank 2 of `ui_original_juno_rows`, `ui_original_controls.h` 19), and jumping the bank would be a mid-performance page change. Phase 2 may *also* light the cell when it happens to be visible.

Fonts (packed Barlow, `X0X_FONTSET=barlow`, `tools/gen_font.py` 183): FONT_S / B / XS fold `i e o h` (and other letters not in `LOWER_KEEP`) to capitals. **Every string in this spec is A–Z, digits, space, and `%`**, so the fold cannot mangle it. Use FONT_T for the `PATCH` cue (already capitals-only, 32–95).

Measured advances (felucca at this font header), inner width of the box `224 − 24 = 200`:

| String | FONT_B | FONT_S | FONT_M | Fits 200? |
|---|---:|---:|---:|---|
| `DRONE A  BRIGHTNESS` (longest name) | 136 | 135 | — | with FONT_M `100` (36) + FONT_S `%` (~15) = ~187, yes |
| `CHICAGO A  CUTOFF` | 117 | 115 | — | yes |
| `UNASSIGNED` | 79 | 78 | 121 | yes, FONT_B |
| two targets on one FONT_B line | 262 | 255 | — | **no** — use two lines |

---

## 4. Exact text

Engine word is `CHICAGO`, `MIAMI`, `JUNO`, or `DRONE` (`BENGINE_N`, `ui.c` 426), taken from the **latched** (or CC1-matched) part’s `original_bass[v].kind`, **not** from the part now on screen (GEN-36A: the sound stays on the latched part; the label must too). Slot is `A` for voice 0, `B` for voice 1. Parameter is `ui_original_name(kind, mod_param(v))`: `CUTOFF`, `WOBBLE`, or `BRIGHTNESS`.

**Percent law**, same as the knob (`ui.c` 1075, 1110):

```
pct = (live * 100 + 63) / 127     /* integer; 0 → 0, 64 → 50, 127 → 100 */
```

The live integer is `mod_ctl[v]` (0..127). The patch integer, for the honesty cue only, is `pref_get` / `proj.sound.v[T_303][v][mod_param(v)]`. Need a tiny production getter for `mod_ctl` (today `engine_bass_perf(..., 2, 1)` is host-only, `engine.c` 1248–1252). HOST ESTIMATE **+8–16 B**.

### 4.1 Single target (physical wheel, or CC1 that hits one bass part)

One line, shipped `draw_readout` layout (`6486–6503`):

- Left, FONT_B, `C_GRAY`: `{ENGINE} {A|B}  {PARAM}`  (two spaces between slot and param)
- Right, FONT_M, `C_WHITE`: the live `pct`
- Right of that, FONT_S, `C_GRAY`: `%`

Examples (these are the strings to paint, and the acceptance fixtures):

| Route | Name | Value at live 64 | Value at live 0 | Value at live 127 |
|---|---|---|---|---|
| Juno voice 0 | `JUNO A  CUTOFF` | `50` `%` | `0` `%` | `100` `%` |
| Juno voice 1 | `JUNO B  CUTOFF` | same law | | |
| Chicago voice 0 | `CHICAGO A  CUTOFF` | same | | |
| Miami voice 0 | `MIAMI A  WOBBLE` | same | | |
| Miami voice 1 | `MIAMI B  WOBBLE` | same | | |
| Drone voice 0 | `DRONE A  BRIGHTNESS` | same | | |

Never `303A`, never `303B`, never `CHI` as a substitute for `CHICAGO` on this box (the header already uses the full engine word). Never `WOBBLE DEPTH` — the shipped name is `WOBBLE`. Never `CC1`.

If `live != patch`, a second line, FONT_T, `C_GRAY`, right-aligned under the value:

```
PATCH  {patch_pct} %
```

The main number remains the **live** percent. Do **not** use the word `SAVED` here: in the overhaul, `SAVED` is only a successful flash write. The pot is `PATCH`. Do not show this line when live equals the pot.

### 4.2 Two targets (CC1, both bass parts on the same channel)

Two lines in the same 224×50 box. Never collapse to one name or one “CC1 64”. Physical wheel cannot produce this (one `cut_part`).

Each line, FONT_S (FONT_B names also fit per line; FONT_S leaves a cleaner two-line measure in 50 px, two × 16 + a 4 px gap):

- Left, FONT_S, `C_GRAY`: `{ENGINE} {A|B}  {PARAM}`
- Right, FONT_S, `C_WHITE` + FONT_S `C_GRAY` `%`: live percent for **that** part (same raw `d2` today, still named twice)

Fixture, 303A = Juno and 303B = Miami, both on MIDI channel 2, CC1 = 64:

```
JUNO A  CUTOFF                50 %
MIAMI B  WOBBLE               50 %
```

Order: voice 0 then voice 1 (303A then 303B). If only one of the two still has `mod_ctl >= 0` on a later frame, fall back to the single-target layout. If live ≠ patch on a line, that line may append `PATCH  nn %` in FONT_T only if it still fits (200 px); if not, omit the PATCH cue on the two-target box — the live names are the honesty that matters, and Phase 2’s cell takes the rest.

### 4.3 Unassigned (physical wheel, no 303 latched)

Name only, FONT_B, `C_GRAY`, vertically centred in the box, left at `bx+12` like a name (or centred — both fit; **use left + 12**, same as `draw_readout`, so unassigned is the same family):

```
UNASSIGNED
```

No number, no `%`, no `CC1`, no parameter name, no `NO MOD TARGET` (the owner’s word is unassigned). No engine word (there is no engine).

**Once per gesture:** on the first accepted `mw` change (`mw != mw_now` after hysteresis) while `cut_part` stays `-1`. Do not retrigger on every ADC step — a 909 performance must not strobe the box. 900 ms then dismiss. A return to rest and a new move is a new gesture (fire once again). Incoming CC1 that hits no bass part is **not** shown (no main-loop signal; do not touch `midi_in`).

---

## 5. Timeout, refresh, and who wins

| Event | Timer | Draw |
|---|---|---|
| Accepted physical `mw` change, target latched | `wheel_until = plat_ms() + 900`, same constant as knobs | Every `ui_frame` while `wheel_until - plat_ms() > 0`, paint the **latest** accepted live value (and names). `smk_wheels` only sets the flag, the snapshot, and the timestamp. It does not blit. |
| Accepted `ext_touch` bit 1 (CC1, main loop, `main_fm1.c` 1102–1107) | Same 900 ms, reset on each seen touch | Same box. Snapshot both parts that reported bit 1 on this poll. |
| Physical unassigned, first step of the gesture | 900 ms, **not** reset by later steps of the same gesture | `UNASSIGNED`, once. |
| Timer expires | `wheel_n = 0` | Box gone. Phase 1: SOUND cells still show the pot (honesty hole until Phase 2). Phase 2: cells show live while `mod_ctl >= 0`. |
| Wheel held (`cut_part >= 0`) across a page / part change | Timer keeps resetting as `mw` changes; **names stay on the latched part** | Do not retarget the label to the new screen’s knob (TRACE test 4). |

**Who wins the 224×50 surface** — one box, one owner, last **qualifying** interaction:

1. **Knob** and **wheel** share the surface. The most recent accepted event (a knob detent that sets `ui.touched`, or an accepted `mw` / `ext_touch`) owns the box and its 900 ms. A knob detent during a held wheel takes the box and shows the **knob’s** param (the player is looking at that knob); the next accepted wheel step takes it back, still labelled as the **latched wheel** param, not the knob. Do not run two boxes.
2. **Fader `say()`** stays on the header. The wheel uses the box, so both can be visible. This is another reason not to put the wheel on `say()`.
3. **Overhaul `UNSAVED` / `SAVE AT STOP` / `Q:` / play icon** stay in the header. The wheel must not call `say()` in the recommended path. A `SAVE AT STOP` outline pill in the main band (already specified in the overhaul) and the wheel box may share band 1; if they would overlap, the wheel box sits at the shipped `(8, 103)` and the pill uses the existing toast y (header-adjacent, y≈22) — they do not collide.
4. **Envelope ADSR graph** (`adsr_touch_active`, 900 ms): a knob-owned ADSR graph already replaces the matrix. If a knob owns the surface, keep that. If the wheel then takes the surface, paint the 224×50 box on band 1 as usual (do not open the ADSR graph for the wheel).

Between frames, `smk_wheels` may accept several hysteresis-qualified steps; only the last snapshot is drawn on the next `ui_frame`. That is the correct “fast sweep” behaviour (TRACE tests 2, 9).

---

## 6. Value units

**Recommend percent**, with the existing knob law `(v*100+63)/127` and unit `%`.

Evidence: `original_value_text` `ui.c` 1075 (Juno, Drone, every param) and 1110 (fallback, which is Chicago CUTOFF and Miami WOBBLE). A raw 0..127 on the wheel next to a knob that says `55 %` would be a new inconsistency. 0 and 127 must still publish (hysteresis bypass, `main_fm1.c` 1055–1058); they display as `0 %` and `100 %`.

---

## 7. At-rest-0 honesty (smallest honest cue)

After release the engine is at live 0, `mod_ctl` is 0, the pot is unchanged, and a naive “CUTOFF 0 %” that then vanishes leaves the SOUND page showing the pot (e.g. `55 %`) while the filter is closed. Do **not** write the pot back (that would change the musical behaviour, which is forbidden). Do **not** call this `SAVED` or `UNSAVED`.

**Phase 1, in the box only (smallest):** while the box is up, the big number is live; if `live != patch`, the dim `PATCH  nn %` line of §4.1 is shown, including on the rest-0 frame (`LIVE` is the big `0 %`, `PATCH` is the pot). 900 ms after rest, the box goes away. This is honest for the gesture and cheap.

**Phase 2, lasting:** `matrix_value` / knob `pref_get` path, for `R_ENG` / `T_303` / `i == engine_bass_mod_param(v)` only, returns `mod_ctl[v]` when `mod_ctl[v] >= 0`. The cell then shows `0 %` after rest, which is true of the engine. Add a FONT_T `LIVE` (or a 1 px amber tick, same family as a p-lock notch, **not** a new glyph table) on that cell while `mod_ctl >= 0`. A knob edit of the same param already sets `mod_ctl` to `-1` and takes the pot path back.

---

## 8. Hook (FMB, `ui.c` + a few lines in `smk_wheels`)

**Set state only** in `smk_wheels` (`main_fm1.c` 1087–1107), main loop, including the between-frames call (2583). **Draw only** in `ui_frame` → `draw_main`, a `draw_wheel_readout()` next to `draw_readout` (or a parameterised reuse of the same box).

Never in `midi_in`, `mod_apply`, `apply_param`, `smk__wheel`, the audio ISR, or the key-scan ISR. Do not change `engine_bass_mod` / `mod_apply` musical behaviour. Do not block or reorder note-on/off.

Suggested state (BSS HOST ESTIMATE 12–20 B): a count (0 / 1 / 2 / unassigned), two part indices, two live values, two patch values, `wheel_until`, and a “unassigned already fired this gesture” flag, cleared when `mw` returns to 0 with `cut_part < 0`.

For CC1, use the values `ext_touch` already reports on the main loop, then read `mod_ctl` via the new getter. For the physical unassigned case, fire on the first `mw != mw_now` with `cut_part < 0`.

---

## 9. Flash / HOST ESTIMATE (against the `budget.md` conflict)

Elite headroom **248 B measured**. Chord namer Tier 1 **285 B measured** — **already over** Elite before this work (`overhaul/budget.md`). These figures are HOST ESTIMATE except those two. Measure at link on this exact head plus the patch. Do not shrink the reserve. Do not evict factory presets.

| Phase | What | Flash HOST ESTIMATE | Pool | BSS |
|---|---|---|---|---|
| FMB’s `say()` (not recommended) | Header toast, 1300 ms, name + live value | **+80 to +180 B** (FMB’s range, agreed as an estimate) | 0 (`ui.msg[64]`) | 0–8 B |
| **Phase 1 (this spec, recommended)** | `draw_wheel_readout` reuse of the 224×50 box; live formatter (existing percent law + `mod_ctl` getter); single / two-line / `UNASSIGNED`; `PATCH` cue; 900 ms; no header wipe | **+120 to +250 B** | 0 if no new buffer | 12–20 B |
| **Phase 2** | Phase 1 + `matrix_value` / knob cell read `mod_ctl` when `>= 0` + `LIVE` mark | **+80 to +150 B** on top of Phase 1; **+200 to +400 B** combined (aligns with FMB’s option B) | 0 | 0 extra if the same snapshot is reused |
| New overlay, new font, SMC popup | Rejected | — | — | — |

Phase 1 already competes with the namer for Elite. If the link is over: keep the box and the three cases; drop the `PATCH` line first (~40 B); drop Phase 2. Do not fall back to a raw `CC1` label to save bytes. Zero Phase-2 is acceptable. A measured `say()` is a last-resort land only if Phase 1 cannot link **and** the header wipe is accepted as a known L regression — record that regression in the implementation note if so.

CPU: no new work in `midi_in` or per-sample paths. `ui_frame` draws one 224×50 box (already in the budget of a knob turn). Fast sweeps must not stretch `ui_frame` enough to delay `smk_grid` / note-off (acceptance below). 0 additional ISR cost.

---

## 10. Acceptance (measurable)

Host tests already cover wheel **sound**. Add UI cases. Device claims need a device measurement; do not claim “no slowdown” without one (TRACE test 10).

1. **Name and value, each engine and route.** Physical wheel, part selected at start: Chicago A/B → `CHICAGO {A\|B}  CUTOFF` + percent of live; Miami → `MIAMI {A\|B}  WOBBLE`; Juno → `JUNO {A\|B}  CUTOFF`; Drone → `DRONE {A\|B}  BRIGHTNESS`. Values at 0, 64, 127 equal `(v*100+63)/127` `%`. 0 errors. Same for CC1 on that part’s channel only (one target).
2. **Two targets.** 303A and 303B on the same MIDI channel, CC1 = 64: both engines apply; the box names **both** engines and **both** params and both percents. 0 frames collapse this to one name. Physical wheel in the same setup still shows only the latched part.
3. **Unassigned, once per physical gesture.** 909 (or 808 or BREAK) selected, wheel swept: `UNASSIGNED`, no parameter write (`mod_ctl` unchanged), no `CC1` substring in `ui.msg` or in the box. A second fire does not occur until rest and a new gesture. 0 strobes during a continuous 909 sweep.
4. **No `CC1` string.** Grep of the painted strings and of `ui.msg` during the fixtures: 0 hits.
5. **Latched identity.** Start on Juno A, change the visible part to Miami B before release: sound and label stay `JUNO A  CUTOFF`. 0 flips to Miami or to the new page’s knob.
6. **Notes during fast sweeps.** Hold a bass note, sweep the wheel and CC1 faster than the frame rate, on Chicago, Miami, Juno, Drone, both 303s: every note-off arrives (keyboard, USB, BLE, vel-0); no stuck gate. `ui_frame` period does not regress against a same-SHA baseline by more than the host-test noise floor (record the numbers). No draw from `midi_in` or `smk__wheel`.
7. **Rest-0 honesty.** After rest, the box (while up) shows live `0 %` and `PATCH  {pot} %` when they differ. `proj.sound` for that param is unchanged. Phase 2: after the box dismisses, the cell shows `0 %` with a `LIVE` mark, not the pot, until a knob edit.
8. **Knob wins, then wheel wins.** Turn a knob during a held wheel: the box shows the knob’s param. Next accepted wheel step: the box returns to the latched wheel param and live value. Fader `say()` does not take the box; header `UNSAVED` remains visible.

---

## 11. Frames

Native 240×240, packed felucca + shipped glyphs, 1× and nearest-neighbour 3×, under `frames/{1x,3x}/`. Chrome is the recommended R header (L’s rail only when dirty; these fixtures are clean). The box is the shipped `draw_readout` geometry.

| File | What |
|---|---|
| `before-juno-seq.png` | JUNO A SEQ, playing, wheel conceptually moving, **no** feedback (honest shipped) |
| `after-single-juno.png` | Same page, box `JUNO A  CUTOFF` `55` `%` (live = pot, one line) |
| `after-single-chicago.png` | CHICAGO A SEQ, `CHICAGO A  CUTOFF` `50` `%` |
| `after-single-miami.png` | MIAMI A SEQ, `MIAMI A  WOBBLE` `40` `%` — never labelled 303 |
| `after-single-drone.png` | DRONE A SEQ, `DRONE A  BRIGHTNESS` `52` `%` |
| `after-rest-0.png` | JUNO A, live `0` `%`, `PATCH  55 %` (JUNO POLY cutoff pot 70) |
| `after-double.png` | Two lines: `JUNO A  CUTOFF  50 %` and `MIAMI B  WOBBLE  50 %` |
| `after-unassigned.png` | 909 A SEQ, box `UNASSIGNED`, no number |

---

## 12. What this spec does not do

- No change to latch, hysteresis, deadband, REC `motion_touch`, or `mod_apply`.
- No restore of the pot on release.
- No new glyphs, no new font, no SMC popup, no new page.
- No §3A claim, no “best in class”, no unmeasured “zero latency”.
- No `piano-roll.md` or `presets-spec.md` edit.
