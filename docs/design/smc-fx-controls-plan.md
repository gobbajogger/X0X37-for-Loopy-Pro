---
title: SMC FX controls design
date: 2026-11-02
type: docs
---

# SMC FX controls design

This plan finishes the existing four-row momentary SMC FX page without changing saved sounds or silently editing shared effects. It distinguishes verified PR750 source behaviour from proposed controls and unresolved decisions. It does not implement or qualify firmware.

## Inputs and authority

The maintained [ROADMAP](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/496c87269349b70f9a900bd95b996d765aa51a80/docs/ROADMAP.md) is PR812's `docs/roadmap-tracker@496c87269349b70f9a900bd95b996d765aa51a80`, file blob `ebb54654b2c21aceec4e81aae39fc5158b3908b5`. Its Master tracker governs current planning. The October 19 narrative retained above that tracker and the older main-branch copy are historical status, not current delivery evidence.

PR750 was verified OPEN/DRAFT through the GitHub API on 2026-11-02, with a final source-pin check at 02:25 UTC:

| Input | Pin |
|---|---|
| PR750 head | `e7e20c8179acd50c28e8994a0123f3fb79c59f88` |
| PR750 base | `wip/one-build@5bff28caba24568cd5070acb99aaf37729cd8323` |
| Comparison | Two commits ahead, zero behind |
| Maintained development workflow inspected | `nightly@66073c68dd80d51a7053432c2c161a8b720a1341` |
| Coordination snapshot inspected | `4a35a6cdaa0f112db6b3f577542138afd170c20d` |
| Published-source reference from ROADMAP | `8be16486e33f65b31c76710e1d825b6aa701c2c9` |

These are distinct source identities. PR750's inspected code is not evidence that the same behaviour is present in the published reference or a later integration candidate. The implementation handoff must refresh its actual source and dependency pins.

The current requirement is held FX plus previous/next arrows to choose that button's effect. HOME plus page arrows changes page only when no FX button is held. This supersedes HOME-plus-button FX assignment in PR750's README. Preserve the separate saved-sound-reset gesture described under ROADMAP M4 in its intended context.

The existing PR750 producer retains implementation ownership. This documentation delivery does not transfer a source path, admit another worker, or authorize firmware integration, publication or hardware operations. Work remains in cloud threads; no Mac operation is part of this stage.

## Existing evidence and source pins

Reuse the [existing independent QA receipt](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/750#issuecomment-6538664532): six compiler/sanitizer runs passed, with 9,578 assertions per run. That evidence covers the real mapping/state helper and table bytes. It excludes combined SMC callbacks, BLE, framebuffer rendering, physical LEDs, audio, full-host qualification and Elite/Pro native builds. No checks were repeated for this design.

| PR750 path | Git blob |
|---|---|
| `firmware/src/app/smc_fx_map.h` | `992bddb40c6bc82da19ec8c4f4b853a28d264e25` |
| `firmware/src/app/smc_fx_map_tables.c` | `c07a524b6345c72df7af97ab35d536c6296422c0` |
| `firmware/src/app/smc_fx_strip.h` | `374105011357aff84688949dc98574e36f81fd36` |
| `firmware/src/app/smc_mixer.c` | `e68540bcce9208d702bdd7b296ec4aef95356b12` |
| `firmware/src/app/engine.c` | `6078fc004bf750167b84ac1cd823a2322a005b1b` |
| `firmware/src/dsp/busfx.h` | `3b1a5627c5689b8ca8f17af257bf87dcb431f27c` |
| `firmware/src/dsp/fxbus.c` | `0da80feb42d60222754b05b5e5f0aee62e36d09f` |
| `firmware/src/app/original_sound.c` | `530a63d2759a5a44ce5dcab60d5978433bf4aa5f` |
| `firmware/src/app/ui.c` | `94657b80198733a9254a606c25ee5935b0996b80` |

## Preserved behaviour

All four physical rows are momentary: REC, SOLO, MUTE and SELECT. Physical note groups use that order; the display uses MUTE, SOLO, REC and SELECT from top to bottom. Retain the existing main-SELECT tap navigation and 180 ms held-activation distinction.

Preserve both independent assignment pages and the physical strip mapping through `part_at()`: 909, 808, BREAK, Chicago, Miami, Reverb, Delay and Master. Internal source-bus identifiers 303A/303B are not a replacement for the current runtime engine identity.

Release removes ownership on the original bus even after navigation or strip reordering. Preserve latest-held playback precedence within each bus, peer-zero precedence across peers, locks, peek restoration, pickup, navigation, note ownership and tail safety. Ending an SMC FX hold must not synthesize a musical NoteOff merely because the UI changed.

Only Chicago/Miami and SMK-37 Elite/Pro are in scope. Preserve SDK V1.2.1, licensing, corresponding-source obligations, external-input boundaries, sample quality and existing model/data guards. Do not import restricted SDK, toolchain, plugin binaries, coefficient headers or captures into this artifact or its source delivery.

ROADMAP R003 already records accepted Chicago/Miami keyboard/SMC reach, on-screen names and real/displayed ranges: 22 Chicago and 18 Miami controls. Do not reopen that acceptance. A new momentary per-button knob interface is separate from those existing controls.

## Default maps and additional choices

Keep the accepted PR750 defaults unchanged. Entries use REC / SOLO / MUTE / SELECT order; `--` means unassigned.

| Bus | Page 1 | Page 2 |
|---|---|---|
| 909 | REV / DLY / STUT / SHUF | -- / -- / -- / -- |
| 808 | DLY / REV / STUT / SHUF | -- / -- / -- / -- |
| BREAK | GATE / REV / DLY / HPF | -- / -- / -- / -- |
| Source bus 303A | HPF / DLY / REV / LPF | G8 / G16 / TRNC / -- |
| Source bus 303B | HPF / LPF / DLY / REV | G16 / G8 / -- / -- |
| Reverb | REV / G8 / G16 / TRNC | GATE / STUT / SHUF / -- |
| Delay | DLY / STUT / SHUF / G16 | G8 / -- / -- / -- |
| Master | G16 / G8 / TRNC / STUT | SHUF / HPF / LPF / -- |

Separate default assignments from the selectable catalogue. Current initialization indexes `SMCFX_SUIT` directly; widening that table alone would populate formerly blank page-two cells and change the defaults.

First consider the existing GPL-covered momentary algorithms. The following catalogue expansion is a PROPOSAL based on inspected call paths; audible suitability is NOT_RUN:

| Bus | Proposed selectable catalogue |
|---|---|
| 909 / 808 | All ten existing momentary effects |
| Chicago / Miami / BREAK | All existing momentary effects except STUT |
| Reverb send input | All existing momentary effects except DLY |
| Delay send input | All existing momentary effects except REV |
| Master | GATE, HPF, LPF, G8, G16, TRNC, STUT, SHUF |

`bfx_part_stut_base()` allocates stutter channels only for 909 and 808; Chicago/Miami/BREAK return `-1`. Send-input throws affect their corresponding send, not the opposite send. Master shaping does not implement REV/DLY throws. Reverb/Delay strip controls operate on aggregated send inputs and must be labelled accordingly.

The exact additional named-effect wishlist beyond the available specification is UNKNOWN. This proposed catalogue is not evidence that every listed addition was individually approved.

## Exact current effect controls and ranges

The inspected engine publication interface carries only a held mask and effect IDs. None of these ten momentary effects currently exposes independent per-button amount, primary or secondary fields. Source constants are not adjustable controls.

| Effect | Verified current behaviour/constants | Existing editable momentary parameters |
|---|---|---|
| GATE | Follower `0.002`; trigger ratio `1.7`, minimum peak `0.02`; opening `0.3`, decay `0.0012`; gate bounded 0-1 | None |
| REV | Held send becomes `1.0`; release returns to saved send | None; decay/tone belong to shared fxbus |
| HPF | Fixed coefficient `0.0395`; effect mix slew `0.01` | None |
| DLY | Held send becomes `1.0`; release returns to saved send | None; time/feedback belong to shared fxbus |
| LPF | Fixed tone coefficient `0.008`; effect mix slew `0.01` | None |
| G8 | Eighth-note period; duty `0.5` | None |
| G16 | Sixteenth-note period; duty `0.5` | None |
| TRNC | Sixteenth-note period; duty `0.38` | None |
| STUT | Sixteenth period; duty `0.22`; chunk `max(sp16/4,8)`; two-sample ring | None |
| SHUF | Sixteenth parity; odd-step phase offset `0.34`; threshold `0.48` | None |

Rhythmic timing reads the existing tempo, clamped internally to 20-300 BPM, with `sp16 = round(44100*15/BPM)`. Held-button editing must not modify global tempo. The inspected implementation does not establish a user-adjustable long capture/repeat length for STUT.

### Proposed held-button knobs

K1 amount, K2 primary and K3 secondary remain a PROPOSAL, not an accepted or implemented universal interface.

The recommended bounded first control is K1 Amount, integer 0-127, in temporary bus-local state. Initial 127 preserves the existing full-strength effect; 0 removes its momentary contribution. Filter contribution, gate depth and send-throw increment can be scaled around the existing algorithms. Exact interpolation, transition smoothing and resource cost need implementation proof; no completed DSP or control test is claimed.

K2/K3 remain unassigned until a source-backed per-effect parameter contract is selected. Display `--` and consume those turns during a held-edit gesture so they cannot reach unrelated page parameters. Do not relabel an unrelated channel cutoff as the momentary HPF's cutoff or expose fixed constants as existing editable ranges.

The intended knob surface, panel K1-K3 versus the SMC's eight relative encoders, is UNKNOWN. These have different input paths and must not be conflated. Resolve that input boundary in the scoped producer handoff.

### Shared controls excluded from implicit per-source editing

| Shared processor | Verified raw controls and mapped ranges |
|---|---|
| Reverb | Decay 0-127 maps to 0.2-0.93; Tone 0-127 maps to 0-1; HPF 0-127; Level 0-127 maps to 0-1.2 |
| Delay | Time 0-12; Feedback 0-127 maps to 0-0.85; Tone 0-127 maps to 0-1; Level 0-127 maps to 0-1.2; HPF 0-127; Type 0-1; Wear 0-127; Ping 0-1 |
| FX master | Distortion 0-7; Drive 0-127; Glue 0-127 maps to 0-1; Volume 0-127 |

Delay divisions are `1/32, 1/16T, 1/16, 1/8T, 1/16., 1/8, 1/4T, 1/8., 1/4, 1/2T, 1/4., 1/2, 1/2.` Actual time folds by halves when the line cannot hold the requested duration. Physical conversion endpoints for HPF and Drive were not established in this design: UNKNOWN.

These controls belong to one shared `T_FX` fxbus, reached through `apply_param(T_FX,...)`. A source-bus hold must not silently edit them. Restoring a cached value after release can also overwrite a legitimate intervening edit; use temporary influence rather than save/write/restore of shared settings.

### Other existing effect candidates

The active original Chicago profile exposes OFF, CHO/FLG, PHASER, RING MOD, COMPRESSOR, DECIMATOR and EQ. `FxType` is 0-6; `FxEd1` and `FxEd2` are 0-127. Miami exposes the existing Crush macro at index 10, raw 0-127. Existing master distortion and compression provide further licensed source candidates.

These are native patch/master processors, not current SMC momentary effects. Chicago cold selection explicitly discards the previous selected processor and tail. Changing its type for a hold and restoring patch bytes does not preserve that state. Keep these candidates outside the bounded momentary extension until their source owner supplies a compatible lifecycle contract. Per-effect physical Edit1/Edit2 meanings beyond the verified macro ranges remain UNKNOWN here.

Do not revive the legacy twelve-type `chicago_fx.h` bridge as the active original-engine interface. Follow the inspected [Chicago compact-selection policy](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/e7e20c8179acd50c28e8994a0123f3fb79c59f88/experiments/original_synth/CHICAGO-COMPACT-POLICY.md).

## Gesture arbitration and state transitions

Capture a physical edit token at eligible key-down: peer, note, original bus, original page, row and link generation. Include blank cells and pending main-SELECT holds, which may not appear in the active-effect ownership table.

With exactly one eligible FX button held by that peer, previous/next arrows change only its captured assignment. Add reverse cycling; current `smcfx_cycle()` advances only forward. The source page arrows are notes `0x62/0x63`; preserve Up/Down knob-bank use under ROADMAP #37 rather than broadening the gesture to all four arrows.

Proposed audition behaviour: update the held effect together with its assignment so selection can be heard during the hold. An initially blank cell stays silent until assigned. This is a proposed live-selection rule, not existing helper behaviour: the current helper latches the effect selected at key-down.

With multiple eligible FX buttons held, preserve existing playback precedence but reject assignment and knob edits with clear MULTI HOLD feedback. This conservative edit policy is a PROPOSAL; the source has per-bus playback order but no complete multi-held edit-target contract. Panel control across multiple peers also needs an explicit target rule before implementation.

HOME plus page arrows changes page only with no eligible FX hold. Otherwise it must not change page, invoke cutoff save/reset or fall through to unrelated navigation. Remove the superseded HOME-plus-FX assignment branch and CYC caption. Preserve M4's HOME-plus-part-SELECT saved-sound reset in its intended context, and preserve M1's SMC BANK L+R PANIC assignment in DAW mode only, never MMC.

The unresolved physical collision is note 6: it is both the SMC HOME alias and the REC-row Delay-strip FX button. If it owns an FX token, literal FX priority means arrows select that button's effect. It cannot simultaneously be treated as HOME with no FX held. Panel HOME is separately identifiable. The producer handoff must settle the controller gesture without silently weakening either rule.

Release removes the original token. Releasing the winning effect reveals the earlier held effect and its own amount. Peer-zero precedence remains unchanged. Publish effect and amount from the same winning peer/button and snapshot the complete tuple coherently at the existing render boundary. The current two-field interface cannot carry the proposed amount unchanged.

## Cleanup, notes, tails and persistence

Ordinary release removes only temporary FX influence, with existing smoothing retained. Send tails continue through the existing fxbus; do not reset their histories or cold-switch a native insert. Preserve notes on their original part through ordinary menu/help/popup navigation until the actual matching key release, as required by ROADMAP #16. Clearing SMC effect ownership is separate from releasing musical notes.

Link loss, reconnect generation change, release-loss recovery, project reset, confirmation blocking and FX-view exit must clear the affected peer's edit/hold tokens and republish remaining owners. Late or stale releases must not clear a newer owner's token. A lost blank-cell release must not leave editing armed. Current `smc_fx_clear()` can return early when its active/pending flags are empty, so the additional edit-token lifecycle requires explicit coverage.

Preset, automation or other live changes underneath a hold remain authoritative after release. Avoid restoring stale saved parameter snapshots. Retain the existing lock and peek destinations and restoration behaviour.

Current assignment maps are peer-local RAM. `smc_fx_save_preset()` saves channel cutoffs, not the 8x4 assignment maps. Durable assignment/amount storage and power-cycle recall are UNKNOWN / NOT_IMPLEMENTED. No storage revision, spare-byte scheme or persistent-write authority is proposed here.

## Implementation boundaries and consumers

| Unit | Affected paths and callers | Boundary and meaningful checks |
|---|---|---|
| Catalogue and default separation | `firmware/src/app/smc_fx_map.h`, `smc_fx_map_tables.c`, `smc_fx_strip.h` | Existing PR750 producer; retain every default/blank cell, forward/reverse choice limits and per-page isolation |
| Held assignment and cleanup | `firmware/src/app/smc_mixer.c`: `smc_button_apply`, `smc_fx_try`, `smc_release`, `smc_fx_release_note`, `smc_fx_clear`, `smc_poll_peer` | Existing producer; original-bus release, blank/pending token, repeats, lost release, link generation, multi-held edits and peer precedence |
| Encoder routing | `smc_knob`; `firmware/src/app/ui.c` panel loop around `enc(EN_K1+i)` and `turn_ref` | Explicit handoff from current UI owner before any overlapping edit; no unrelated page edits and no duplicate fader destination |
| Amount publication/application | `firmware/src/app/engine.h`, `engine.c`: `engine_smc_busfx`, `bfx_snapshot`, `mix_targets`, shaping callers; `firmware/src/dsp/busfx.h` | Explicit engine-path handoff; coherent winner tuple, endpoints, release smoothing, unchanged saved/shared values and note/tail safety |
| Captions and actual screen fixtures | `smc_button_caption`, `smc_ui_grid`, `smc_ui_draw_peer`; existing framebuffer harness | Existing UI presentation; truthful labels and raw production-framebuffer evidence |
| Qualification | Existing host suites and exact Elite/Pro SDK V1.2.1 closure | Existing build and independent-QA owners; source, host, native, package, publication and device states remain separate |

These units are a plan, not assigned new implementation work. PR750 ownership is unchanged. Preserve the current cohesive `ui.c` held-note/PARA producer and separate visual-reference scope. Engine/shared-FX attribution retains its current priority and serialized measurement owner; this design authorizes no additional profiler, benchmark or optimizer.

No changes are proposed to Chicago/Miami kernels, BLE transport, storage layout, `.github/`, installer or publication paths. Sole firmware integration and publication ownership remain unchanged.

## Acceptance and later proof

| Positive acceptance | Required negative acceptance |
|---|---|
| All 32 buttons resolve correctly on both pages | No forced LPF, wrong-bus release or unrelated musical NoteOff |
| One held button plus previous/next changes only its captured cell | HOME-plus-button does not assign FX; saved-sound reset retains its separate intended context |
| Both pages recall unchanged defaults and RAM edits | Catalogue expansion does not fill default blanks |
| Proposed K1 endpoints preserve no contribution/full existing behaviour | No saved mixer, shared FX, global tempo or native patch mutation |
| Winning release restores earlier held ownership and its parameters | Peer-one edits cannot overwrite peer-zero parameters |
| Blank held cell can be assigned without triggering another cell | Blank press/release cannot steal a live effect or leave editing armed |
| Main SELECT retains tap/hold behaviour | Editing cannot replay short-tap navigation on release |
| Disconnect and release-loss clear temporary state | Duplicate NoteOn, stale generation and late NoteOff cannot rearm or clear the wrong owner |
| Locks and peek retain captured destinations | Held turns cannot edit unrelated page controls |
| Notes and existing tails continue correctly | No stolen voice, discarded native insert tail or reset caused by effect reassignment |

Extend existing tests rather than create another audit: `tests/host/smc_fx_two_page_strip_test.c`, `tests/host/smc_fx_preset_preserve_test.py`, `tests/host/smc_fx_test.c`, and the existing UI, lock, peek, multi-peer, interaction and release-loss suites. Reuse accepted unchanged exact-input evidence; run only the changed closure and required final gates.

All execution for this design is NOT_RUN. No firmware test, native build, image rendering, performance measurement or device result is claimed.

The maintained ROADMAP requires the pinned final candidate's host suite to have ZERO FAIL, ZERO SKIP and ZERO NOT_RUN, plus Elite and Pro SDK V1.2.1 builds with exit 0 in the required release configuration. M10 requires actual measured free pool headroom separately for Elite and Pro, at least 8,192 B each, at every applicable PR/step. Missing, stale, inferred, NOT_RUN or subthreshold measurements block the affected qualification. No measurement or waiver is supplied by this design.

Preserve audio identity except accepted differences, intended visuals and control behaviour. Later severe-load qualification under #107 must declare its exact workload and duration, combine both synths, drums, FX tails, MIDI, SMC and active UI, and report observed dropouts, lost events, worst-case deadline margins and input latency. Host or synthetic timing is not real-device acceptance. This plan starts no such run.

## Required visual evidence

Reuse M6's [approved effect mockup](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/5bb2bca2446b04ec643d2ecab20859fd62cf311b/docs/mockups/items-4-6-7-0.0.24/item6-after-effect.png) and the approved #41/#736 presentation. Do not reopen unchanged aesthetic acceptance. The accompanying drum-fill assignment remains a separate proposal.

PR750's three `docs/feature41-ui/*reference.png` files are diagrammatic references, not framebuffer proof. The later implementation must supply actual production-rendered 240x240 before/after frames for page 1, page 2 including blanks, held Chicago/source-303A, arrow assignment and amount feedback, multi-held rejection, peer shadowing, and locked/peek context.

Record exact source pins, fixture inputs and raw-image hashes; put comparison labels outside raw framebuffer pixels. `tests/host/smc_fx_framebuffer_capture.c` currently covers HOME and one FX fixture only and needs the additional states. Follow mockup, internal independent review, implementation and actual affected-render/interaction validation. No frames were rendered for this design.

## Release scope and stop conditions

M9/M12 preserve all already-built features and selected fixes while explicitly keeping unselected, unbuilt work QUEUED. This plan does not select new SMC work into a release. M12 must pass on the exact final candidate/build tree and independently accepted ROADMAP/release-selection pins; a documentation file or scoped source PASS is not release completeness.

M11 requires the existing publisher to publish passing NIGHTLY builds automatically after all pre-publication gates pass, without another routine approval wait. Live site/catalog/package bytes and prior-version preservation are verified after publication. Agents never install or flash; installation is owner-operated through NIGHTLY. Production retains its separate same-byte, matching-model hardware and promotion requirements. No publication or hardware operation is part of this documentation stage.

The next consumer is the parent coordinator and existing PR750 producer. An explicit scoped handoff must resolve the note-6 HOME collision, intended knob surface, multi-held edit policy, exact additional named effects, any K2/K3 contract and persistence requirement. It must also release any overlapping UI/engine paths before a fresh implementation stage. The proposed controls are not implementation-ready while those behaviour decisions remain unresolved.

Source discovery, independent QA, firmware builds and mockup rendering were not repeated for this documentation delivery. Public source pins and existing accepted evidence are retained; no private conversation content or restricted payload is included.
