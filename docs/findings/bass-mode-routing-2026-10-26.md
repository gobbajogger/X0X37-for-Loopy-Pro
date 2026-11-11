# Bass-mode audit: Miami/WBL1 and Chicago/native303 (2026-10-26)

Base: firmware `nightly` at `3062e0c52638d9063ee84bd36940a36e2269a0ba`.
Scope: source-only corrections on an isolated branch, **not** a validated or installed build.

## Source defects addressed

1. `303A` boots Miami/WBL1; `303B` is Chicago/native303. The same `T_303`
   ordinal previously dispatched Chicago auxiliary pots and WBL1 parameters
   interchangeably. Expose **23 WBL1 parameters**, including Bass Split, versus
   **32 Chicago parameters**. Reject phantom Chicago descriptors on Miami and
   dispatch WBL1 writes before Chicago's FX/arp/bend/damper handlers.
2. `engine_sound_defaults` initialized the Miami sound cell with Chicago
   cutoff/drive values; for example, slot 0 (Miami Carrier Wave max 1) received
   Chicago Cutoff 48. This fails the same range rule project import validates.
   Keep Miami's WBL1 metadata defaults and apply acid overrides only to native303.
3. Miami's pitch wheel wrote `CHI_PBEND` to the dormant native engine, not
   Miami `W_PITCH_BEND`. The live bend and spring-return now use the selected
   sound engine's control, with original-voice gesture ownership across navigation.
4. `wobble_foundation_set` normalized all 0/1 UI switches as `value/127`,
   which never crosses the DSP's 0.5 threshold. Carrier Wave, XMod Wave,
   Wobble Shape and Damper now use actual off/on values.
5. Bass Split (60–250 Hz, approx. 90 Hz default) existed in the Miami DSP at
   index 22, but did not have complete pot metadata, count, or a physical
   UI control. The fourth knob of MIAMI AUX exposes it with default 36/127.
   The 16-byte Miami AU bank layout stays unchanged.
6. Miami live/MIDI note-off bypassed its own Damper by invoking the all-notes-off
   panic helper. Use the WBL1 note-off for live release so its own damper can
   defer that gate. Sequenced `s_boff` and transport stop retain immediate off.

## Intentionally unchanged

- Chicago mod wheel **replaces** native cutoff 0–127; down closes, up opens,
  stored knob stays untouched. This is the owner's explicit behavior in PR #348,
  superseding older `BEHAVIOR.md` GEN-36 wording. No automatic base restore
  or mod-wheel remapping is included here.
- Chicago live-arp and FX routing, native ADSR, sequencer ties/random,
  stored pattern format, and mixed sample/tempo generation were not redesigned.
- Miami preset AU bank stays 16 bytes; no license/vendor data bundled.

## Follow-on ownership and validation

- **Existing-project migration requires separate schema owner review.** Existing
  sound rev 10 was authored with incompatible native303 and Miami slot meanings.
  There is no stored per-voice engine ID to safely convert every old project.
  In particular, a previously saved Miami slot 22 may contain 0 rather than
  the new default Bass Split 36. Do not blindly rewrite user projects, discard
  old parameters, or bump the shared stream format in this scoped fix. Coordinate
  with PR #249's recorder/snapshot lifecycle integrator before merging.
- REC held-duration and repeated RANDOM/PINGPONG timing remain owned by the
  frozen PR #247 lane, not fixed by this PR. Chicago acoustic/velocity-to-cutoff
  comparison remains separate from this deterministic routing repair.
- Added Miami host regressions for parameter layout/ranges/defaults, binary
  switch behavior, Bass Split, both pitch-wheel paths, and Damper gate release.
  Existing native wheel matrix is intentionally unchanged and must still pass.
- **Not run in this tool environment:** exact host selectors
  `smk-wheel-mode`, `miami-mw-wobble`, relevant project/engine tests, and
  Elite/Pro native `DEV CHECKED` link/budget validation. Independent tests,
  saved-project compatibility review, target XIP/static RAM/pool >= 8192,
  and device/audio checks are required before any merge or installer candidate.
- No hardware flash, installer upload, protected-branch merge, or production
  change is authorized by this audit branch.

## Reason / purpose / user effect

Why: Miami and Chicago are different DSP backends but shared legacy ordinal
routing and sound layout. Fixing those boundaries prevents invalid presets,
nonfunctional switches, invisible bend gestures and missing Bass Split controls.
Expected user effect **after qualification and integration**: Miami sounds and
saves valid fresh defaults, its pitch wheel works, binary controls and sustain
operate, Bass Split is adjustable; Chicago continues its intentional behavior.
