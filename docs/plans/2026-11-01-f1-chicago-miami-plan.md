---
title: F1 Chicago and Miami controls, effects and presets
date: 2026-11-01
type: feat
artifact_contract: ce-unified-plan/v1
artifact_readiness: implementation-ready
product_contract_source: owner-F1-brief
execution: code
---
# F1 implementation plan

## Goal Capsule
Musicians can select Chicago or Miami, edit their agreed controls, and recall
every factory sound with its intended effects and pattern binding.
Means: port the named implementations into build/0.0.23, preserving saved indexes.
Deliver one draft PR. Owner approval precedes merge. Execution evidence remains
pending wherever the target AGENTS.md prohibits builds/tests in Codex.

## Product Contract
### Summary
Complete only real gaps in rows 1–7, 9, 10, 21 and 106.
### Problem Frame
The base admits seven Chicago FX and 55 factory programs; the other five agreed
inserts and 26 factory programs cannot be selected. Other listed controls already
have source and tests, which need exact-candidate validation rather than a rewrite.
### Requirements
- R1: Two selectable original engines, with fidelity and their full agreed controls
  (rows 1–3, 9). Preserve the existing Chicago ADSR; no Miami ADSR.
- R2: Own performance controls: pitch bend 64 is exactly neutral, modulation uses
  engine_bass_mod, and pedal ownership stays with its original voice (row 4).
- R3: All twelve Chicago FX choices and all 81 Chicago/30 Miami factory presets;
  keep Chicago visible indexes 0–54 and FX choices 0–6 stable (rows 5–6).
- R4: Preserve engine/sound pattern binding, real names and the existing queued
  click-free preset adoption (rows 7, 10). Finish only test-proven gaps in #629.
- R5: Preserve Miami's eight sync divisions, four shape pads, and inert empty pads
  (row 21). Verify opt-out and ownership negatives using existing tests.
- R6: Measure device idle CPU per engine against <=20%, after cold idle and release
  tails (row 106). Host timing is not target CPU proof.
- R7: Obtain real 240x240 host-framebuffer BEFORE/AFTER for every visual change.
  No drawings or old captures may substitute; no new mockup is required.
- R8: SDK V1.2.1, Elite and Pro, X0X_DEV_PADCAL=0; record static free RAM against
  2352 B and explain any decrease. Never touch main, .github/, Actions, main_fm1.c,
  FM-1 targets, protected identity sectors, or device writes.

## Planning Contract
### Key Technical Decisions
- KTD1: Use ae0f05116 compact inserts with c3e1f7f8a/000bd821c short names.
  Its 4096 B arena preserves the base's history allocation. Do not substitute
  fd1af79b3's larger Hall arena; port its factory mapping instead.
- KTD2: Do not use the archived insert idle shortcut: it reports success without
  advancing delay heads/LFO phase. Refuse that shortcut for restored inserts so
  the normal DSP path preserves state. Target idle performance remains measured
  acceptance, not an inferred pass.
- KTD3: Retain base implementations for performance controls, pads, preset
  switching and pattern binding. Ancestry and source comparisons are provenance,
  not runtime test results. d8316b0d2/8d314087a are semantically present even where
  the source commit is not an ancestor; 11d87db08 is an ancestor.
- KTD4: Write/port tests before implementation; do not run them here. The owner's
  explicit fallback and target AGENTS.md override execution steps in generic TDD.
### Scope and risks
One source owner writes the isolated F1 branch; independent code reviewers inspect
the frozen candidate. B2 was not found among PRs targeting build/0.0.23, or branch
searches for B2/dropout. Base is bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2.
The compact Hall uses Q12 allpasses and BPM Delay uses half-rate storage, as in the
explicitly named source. These are fidelity limits to validate, not new DSP design.
No simultaneous retained tails for deselected algorithms are introduced.

## Implementation Units
### U1. Port effect and factory regression coverage first
Files: tests/host/chicago_fx2_inserts_test.c and .sh;
tests/host/original_sound_cell_test.c; tests/host/original_release_integration_test.c;
tests/host/original_release_ui_test.c; tests/run_tests.sh.
Requirements: R1, R3, R7. Dependencies: none.
Port ae0f05116/8233297d8/000bd821c tests and fd1af79b3's complete factory bijection
checks. Extend real UI capture coverage to all FX choices and restored names.
Scenarios: impulse/tail/reset per insert, live BPM changes, stereo preservation,
part isolation, unchanged first 55 factories, all 81 unique source programs,
invalid factory/type refusal, and each restored FX name fitting its real cell.
Add a differential idle-then-note case that catches skipped phase/head advancement.
Verification: registered selectors, base failure and candidate success on the
authorized runner; no test expectations weakened or removed.

### U2. Port the existing compact DSP and complete factory selection
Files: experiments/original_synth/chicago_fx2_compact.c and .h;
experiments/original_synth/chicago_fx2_inserts.h;
firmware/src/app/original_sound.c and .h; firmware/src/app/factory_banks.c and .h.
Requirements: R3. Dependencies: U1. Follow KTD1–KTD3.
Port the named implementation; preserve base exact-idle paths for original seven
algorithms. Append restored factories without changing schema or old mappings.
Verification: U1 coverage; unchanged algorithm PCM/state and full build RAM checks
run outside Codex. Record every port, already-present part and deliberate omission.

### U3. Record row evidence and hand off the exact draft candidate
Files: docs/engineering-delivery/f1-chicago-miami.md; docs/FEATURE-REGISTRY.md;
experiments/original_synth/CHICAGO-COMPACT-POLICY.md.
Requirements: R1–R8. Dependencies: U2.
Map each row to existing/new tests or a concrete device checklist. Give exact
before/after capture scenarios, native model commands and idle measurement
conditions. Record NOT_RUN/NOT_MEASURED honestly and preserve independent review.
Verification: exact changed-file readback, author identity, one draft PR into the
verified base; no protected paths or binaries in its diff.

## Verification Contract
Codex performs source/provenance/diff review only. The authorized external runner
must run the full registered host suite, dedicated insert/state/UI cases, original
sound/wheel/preset/pattern integration, then Elite and Pro SDK V1.2.1 builds with
X0X_DEV_PADCAL=0. Obtain real 240x240 before/after PNGs from the host renderer and
measure idle CPU for each engine on the exact model/package candidate. Keep all
unrun results pending; neither source review nor historical evidence is a PASS.

## Definition of Done
Source stage: one draft PR, exact remote head readback, port ledger, tests written
and registered, independent code review, per-row checklist and explicit evidence
gaps. Remove scratch work from the PR. Acceptance stage: full host 0 fail/0 skip,
both native builds and RAM budgets, real framebuffer PNGs, measured per-engine
idle <=20%, fidelity/click checks, then owner approval before merge.
