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
