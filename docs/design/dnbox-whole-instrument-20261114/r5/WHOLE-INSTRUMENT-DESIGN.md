# R5 whole-instrument revision

This remains the full instrument overhaul. It is a design proposal, not firmware. R4 `791c6399` is preserved as reviewed history and is superseded by the owner's feedback. The candidate identity and independent-review disposition are recorded in the delivery receipt.

The primary outcome is an audible musical idea, an intentional correction, another take, and recoverable preservation. Seeing a badge, refusing an unsafe action, or passing a layout check is not that outcome.

## What already makes the firmware useful

Keep the admitted full37 live synth path, meaningful key/voice ownership, and immediate sequence surface, 11 drum-track selectors, four directly bound lower controls, engine-specific sparse columns, simultaneous neighboring parameter rows, reversible row navigation, independent remembered pages, native parameter diagrams and full-row Chicago ADSR. Keep HOME's TEMPO, SWING, FILTER and PUMP, the 11-bar Song overview and both BAR pages. Engine labels already derive actual engine kind and slot identity. These are retained baseline functions, not R5 inventions.

The LCD, sixteen RGB pads, illuminated buttons and physical inputs form one interface. A keybed highlight in the browser is an inspection aid: the actual keybed has no LEDs. Normal pad content, playhead inversion, held-input priority and clip host colours retain their source owners. REC means armed. Save input acknowledgement is not durable success.

## One visual and control grammar

The 240×240 screen keeps its current dense musical surface and four-column knob geometry. The top 20 pixels contain actual view/engine, pattern, pending target when present, tempo or WAIT, transport, REC, dirty/failure and power. This uses compact familiar symbols, not a new decorative badge or large text banners. The current five-lane musical pattern thumbnail and number are retained. Every remaining band contains musical content, control feedback, a useful current hint, or necessary separation for legibility.

Power occupies the same top-right rectangle on every locally owned page and overlay. It cannot disappear merely because a matrix rather than a card layout rendered. Native source exposes only 0–3 bars, USB host power, or unknown. Those are the normal fixtures. A percentage layout study, if supplied, is explicitly simulated; no exact percentage or charging state is inferred from ADC thresholds. The reserved Settings component needs the common-header interface supplied to its owner; its source is not rewritten here.

Existing current glyph families remain the default for envelope, wave, filter, amount, percussion parameters and performance controls. New recognizable drum-instrument pictograms replace BD/SD track abbreviations; selected names use only genuinely unmapped areas. Juno keeps its existing large values, four parameter labels and neutral bars. Optional shared-family thumbnails use only the unused row-heading gutter, including an ADSR cue with Juno's own laws. The rejected small-value/full-row replacement is not retained. These thumbnails are extensions, not pre-existing dedicated Juno PART glyphs. Chicago continues to use its source native projection and compiled-VCA law. No wall-time cursor stands in for a live voice.

Native parameter IDs, target, voice, pattern and input owner determine value, cue and edit scope together. A drum-local ID can never enter a synth formatter. Sound pages and mixer aliases share the same underlying raw value. Held inspection without a lock shows base and suppresses motion; release returns to the proper base/motion owner. MIX hold does not acquire a step-lock scope. Existing-hit deletion resolves on short release, never immediately on hold-down.

## Whole musical journeys

The executable source/control inventory is [whole-journey-audit.json](evidence/whole-journey-audit.json): 20 musical groups cover J01–J39. Its counts describe actual source routes from stated starts; they do not establish speed or usability acceptance.

| Area | Retain and unify | Real proposed benefit | Completion evidence still required |
|---|---|---|---|
| First beat, 909/808 | 16 columns, 11 tracks plus accent, all lower controls, selection/audition/write distinctions | Recognizable instrument identities with no added navigation; consistent target and light ownership | Same-start track recognition and audible beat/correction |
| 64-step sequencing | Length/rate, four pages, direction, octave/page controls, held chance/velocity/locks | One persistent scope/target grammar across LCD and pads | Longer-pattern editing and original-release return |
| Chicago/Miami | Every engine-specific sparse group, SOUND/FX, native diagrams, current navigation/memory | Consistent hierarchy and truthful native glyph feedback, no substitute oscillators | Bound source projections and current equivalent state comparisons |
| Juno | Current 16 controls, four rows plus SOUND, polyphonic playing context | Retain large readouts; source-family cues in unused headings, without replacing the ADSR control row | Current Juno before capture, new extension exercise, authentic J6 fidelity remains separate |
| Drone archives | Stable IDs and saved cells; owner has deferred audible DSP | Explicit unavailable state and recoverable selection rather than silent conversion | Firmware-owner availability result and reserved CP1 interface |
| BREAK | Current bank/loop/slice and performance controls | Distinguish stored slice choice from momentary override and release destination using existing symbols | Current waveform/state capture and complete slice/release phrase |
| Performance, MIX, FX | All four lower controls, 11-track kit mixes, five-part mixer, native FX vocabulary | Same destination across LCD, knob, pads and mute feedback; compact top status | Complete live task and pressure/collision cases |
| Presets/generation | Direct current routes and intended versus applied sound ownership | Deliberate exploration and return without losing the phrase | Reserved pending/Keep result; generator preview is a proposed capability, not current behavior |
| Pattern/arrangement | Current/sounding/queued distinction, transient chain, neighboring Song bars/repeats | Clear live-chain versus durable Song outcome and correction path | Whole arrangement replay and saved recovery; future chain conversion is not silently assumed |
| Recording/motion | Current stopped/running entry, arm, cursor, p-lock/motion owner, default recorded-velocity limit | Explicit destination and mode, correction/replay/retry in one journey | CP2 selected strategy then CP3 lifecycle; no invented exclusive capture class |
| Save/LOAD/recovery | Real readiness, blockers, refusal, cancel/confirm and transport consequences | A preserved phrase and recoverable retry, not merely a safe refusal | Actual durability/recovery; CP1 result cases remain reserved |
| Clock/panic | Actual preference, WAIT/loss/rejoin and scoped notes-off | Static STOP/WAIT LCD cue, distinct hardware WAIT lamp, clear next playable note | Current package event exercise; no wall-clock simulated beat proof |
| SMC/Loopy | Source follow/lock/peek, held target, host feedback and original release | Clear local selection versus connected target without stealing either | Current native connected captures and controlled host transaction |
| Settings/power | Accepted R2 component, actions and routes remain owned separately | Shared compact top-status contract and retained musical return context | Component-owner adoption; held-note continuity is not solved by KEY RETIRED |

## Retained functions and action counts

No mapped lower control is removed to make space. Empty/unmapped positions stay aligned with their physical knobs. Current parameter paging and SCALE list entry receive no new confirmation. A fixed current engine label or a repaired prototype edit is not a firmware improvement. The drum audit provides 16 retention groups and 36 route counts. Every later layout delta must name the displaced function, replacement location and same-start action delta; retain current behavior when a benefit cannot outweigh a negligible practical tradeoff.

The full 37-key hardware remains explicit. Legacy X0X button roles use physical F3–G5 (27 keys,16 white/11 black). The admitted live synth path separately plays all37 keys in PART or SEQ Free Play with REC off; its source gates exclude questions, Global/other views, clip/SMC ownership and busy/testing states. Clip keyboard also uses all37. Preserve that existing full37 musical path; do not confuse the legacy27 control bitmap with musical capability. Do not silently present 37-key recording as current, confuse pad-grid with pad-piano mappings, or invent keybed lamps. K1 is master; K2–K4 retain selection/part/preset roles and K5–K8 remain the four page controls. Printed physical captions are immutable.

## Source, comparison and acceptance

Accepted source `d158444101d569f9ee64ba05d7cb54f24de1be30` has the same UI/glyph files as `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`. Existing actual Elite/Pro 0.0.26 captures executed3c41; they are not d158 execution or R5 firmware. Later `e581333f` corrects masked clip-question cues; `ea10fb14` is the Drone archive/DSP-profile component, not a shipping integrated UI. Historical79280/0.0.25.x evidence stays historical.

Before/after pages must identify same task, page, target, values and source. Unmatched values, phase, font or missing native captures are stated; a fabricated before is forbidden. Native PNGs and a standard PDF are the owner-facing delivery; the interactive HTML remains for inspection and validation. Prior R4 static suitability does not accept R5. The same existing independent reviewer receives a frozen successor and affected tests, not prior scores or desired results.

The seven CP1 cases stay with their incumbent. CP1 decision precedes CP2 selected capture strategy, which precedes CP3 lifecycle. Same-hardware comparator adoption and DF1 whole-musical acceptance remain open. They do not stop disjoint ready visual work, but cannot be closed by render counts. No firmware source edit/build, device action, release or merge is part of this design.
