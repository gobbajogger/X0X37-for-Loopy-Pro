# BREAK feature lane

Branch: `codex/break-feat`. Required base: `e888d046bd16d7f9f64e9c824ca4027570bb38fe`.
Scope: local commits only. No push, device access, install, flash, bootloader or protected-region work. Chicago/Miami and project revision 13 stay in place.

## Progress

- Intake: clean required base verified. Read feature-map items 24–29 and their original requirements. Other lanes retain their owners; no files in their directories are edited.
- PR #637: fetched head `f25f2fd4`; already an ancestor of the base. Adaptive DSP, controls, project encoding and CPU0 MIDI handoff are present. Regression verification is pending; no duplicate replay is needed.
- PR #646: fetched head `55f631b2`; ported its generator and output-error regression with the suite entry. Docker output regression: 6 tests PASS, including `/dev/full` and per-export open/write failures.
- Private build inputs: copied the twelve pinned headers from `../chicago-opt` into this worktree, untracked. They must never be staged.

## Feature items

- 24 — partial: independent A/B counts already present from #637; host verification pending.
- 25 — partial: adaptive C0 input and keyboard count/page navigation already present; host verification pending.
- 26 — partial: accepted-hit MIDI pulses and CPU0 transport ownership already present; host verification pending.
- 27 — in progress: add a transient BREAK pitch-wheel route without changing saved pitch or revision 13.
- 28 — in progress: replace the HALF macro with bounded-memory jungle grain repetition, preserving sequencer timing.
- 29 — blocked: current repository search found no exact recorded owner selection with available authorized stock assets. Keep the current set. The external requirements name three loops and bank `a77e0a2c`, but that is not an available, provenance-verified bank in this checkout. No personal files are searched or copied.

## Validation

Host tests: pending. Docker firmware build (`X0X_ORIGINAL_SYNTH=1`, Elite): pending. Flash free and RAM/pool free: pending. Device and musical acceptance: not run; device access is forbidden for this lane.

Text-only UI proposal: replace the key-10 HALF hint with JUNGLE, preserving the approved layout and the other macros. Independent review found no blocking issue; actual font bounds end at x=207 within x=236. Implementation and actual-render checks follow.

## Implementation checkpoint

- PR #646 is committed as `0ecdd3ab`. Its initial Docker regression passed all six tests. Added current-stock identity preservation coverage afterward; a final Linux rerun follows.
- Items 24–26 use inherited #637, now with shipping-profile regression runners. Controls/MIDI: 5,074 assertions PASS. Storage: 628 assertions PASS; revision/stream 13 only, no migration added. Actual HAL pulse model: 27,348 assertions PASS. Existing BREAK suite and factory-pack tests pass with corrected host compilation.
- Item 27: implemented physical and incoming MIDI bend, +/-12 semitones, runtime-only. Physical gesture ownership survives navigation; external disconnect/channel changes release their bend. Latest-value mailbox protects wheel-centre updates under CQ flood. Tests cover the actual extracted native scanner and queued production engine.
- Item 28: implemented 90 ms jungle grains with half source progression and 2 ms crossfades. Sequencer/pin/stutter timing and MIDI hit counts remain intact. Independent actual-render review passed. Independent source review found an uncrossfaded short-slice boundary; repaired by retaining dry forward tail continuity and adding exact-PCM boundary/engagement regressions. Current audio/lifecycle regression: 2,413,370 assertions PASS.
- Build inputs: all twelve private headers verified against the pinned manifest. They remain untracked/ignored, excluded from all staged paths.
- Environment attempts: initial GCC BREAK suite FAILed on the baseline fastmath weak-inline warning. Initial macOS BREAK script FAILed because its standalone compile omitted X0X_HOST and ignored CC. The harness now defines host mode and respects CC; Clang BREAK suite PASS. Initial integration runner used system Python 3.9 (unsupported by the build module); corrected to Homebrew Python 3.15. No production guard/migration changes were made.
- Target attempt 1 was stopped because host generation could share build/gen. It is incomplete and is not validation. The new unique `break-feat-firmware` container has a separate bind-mounted build output under `build/break-feat/target-output`; host tests cannot overwrite its generated headers. Source/SDK/toolchain remain within the authorized mount scope and no device is mounted.
- Full host suite, sanitizers, final Linux generator checks and isolated target build are running or pending. Flash/RAM/pool figures remain pending. Item 29 remains blocked on the unavailable verified owner bank; current 909 GR / 909 FL set remains unchanged.
