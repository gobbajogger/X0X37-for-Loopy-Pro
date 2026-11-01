# OPT-C: UI frame path

Source candidate only. No builds or tests run in Codex, by owner instruction.
The Orchestrator owns execution and acceptance on the box.

## Source and scope

- Branch: `opt/0.0.24-ui-frame`.
- Pinned parent: `ee89337b36c62002908e6729b02ad87864fb7c52` (#789).
- Intake pin was `02c5224d12c7a96c0061fea71f13de04de58741b`. #789 advanced
  by one pattern-cue popup commit during authoring. Its `ui.c` bytes are unchanged;
  the final parent preserves that commit and its SMC changes.
- #789 was still draft/unmerged during source intake. The owner's follow-up
  permits a stacked base; target its branch `chat/smc-dedicated-popup-023`
  until it merges, then retarget to `build/0.0.23`.
- C1: hash rows first; reject an unchanged main/knob strip before column work.
- C3: visit drum hits in kit/voice order; preserve the volatile decay expression.
- C2: omitted. No complete input signature or long-session proof is available.
- Preserve screen pixels, ordered blits, audio and meter/scope traces. Add no
  persistent storage. No DSP, FM-1, device, bootloader, `.github/` or Actions edits.

## Implementation plan

1. Inspect the pinned production path and existing host fixture. Check historical
   sources before changing it. Write the identity harness and deliberate mutations
   before the production patch. Do not execute them in Codex.
2. Change only the C1/C3 code in `firmware/src/app/ui.c`.
3. Review the source and harness, deliver a draft PR and verify the remote SHA.
4. Orchestrator: prove the mutations fail, establish base determinism, compare
   base/candidate, measure gain and qualify both native targets. Drop a change
   with no measurable gain; do not merge before all acceptance evidence exists.

## Reuse audit

The pinned base already shares `cv_commit_rows` between main and knob strips;
there is no second implementation to port. It still calculates row and column
hashes together. It still decays hits through division/modulo indexing.
PR764's UI diff, PR738 head `2ee4f90e089c53d502a8b8494c328e664608ea7e`,
`9e237cce` and `d7f156ec` were inspected. The latter three retain the flat
decay loop and lack the current shared `cv_commit_rows` routine. Reuse the
current renderer and `smc_ui_test.c` fixture; no renderer or presentation rewrite.
`archive-missed-0.0.23.md` was not found in the pinned tree or default-branch
code search. This does not establish that it is absent from other owner records.

## Box commands and required evidence

From a complete checkout of the delivered head (not the scoped source checkout
used for authoring):

```sh
python3 tests/host/ui_frame_identity_test.py --out build/host/opt-c-identity
sh tests/run_tests.sh
```

The identity runner reuses `smc_ui_test.sh` and production `ui.c`/`gfx.c` and
engine sources. It prepares instrumented scratch copies, never edits production
inputs, and compares per-event SHA-256 logs. It runs base twice, checks deliberate
wrong blit/decay changes, then compares candidate. Source mutation anchor mismatch,
build failure, empty output or missing event classes is a failure, not a PASS.
Default session length is 4096 frames per palette; increase with `--frames`.
Both palettes include asserted active fader/knob/mute/pattern-cue popups,
expiry and controlled framebuffer restoration, plus a cold zero-output idle.
`--base-ui PATH` permits an exact exported parent `ui.c` when the parent Git object
is unavailable; the runner verifies its Git blob identity.

Retain `*.events.jsonl`, `*.summary.json`, `*.build.log`, `*.timing.jsonl` and
`*.png`. Logs include canvas pixels before commit/after draw, framebuffer after
each frame, every ordered blit with rectangle and pixel SHA-256, pre-volume and
post-volume raw int32 stereo audio SHA-256 and meter/hit/scope traces. Timing is
separate from instrumented identity capture. Host instructions are reported only
where the existing host counter supports them; x86 TSC deltas are labelled TSC
ticks, not retired instructions or device cycles.
Samples alternate base/candidate. Cases cover unchanged/changing main and knob
strips, isolated zero/active hit decay, idle UI frames and playback with audio.
Active decay includes identical periodic hit reseeding in both inputs. Timing
uses the host fixture's `-O1` build and disabled observers; it does not establish
native firmware speed. Keep both changing-strip costs and whole-frame results.

Build Elite and Pro with the repository's current SDK V1.2.1 recipe and
`X0X_DEV_PADCAL=0`. Record code size before/after and free static RAM for each.
Do not build FM-1. Both targets must retain at least the 0.0.23 baseline of
2352 B free RAM. Also compare against the exact #789 parent. If a device is
available, record PERF CPU% average/peak. Do not infer hardware permission.

Attach real 240x240 PNGs from the box if any visual difference is found; any
unintended pixel/blit difference fails this optimization. No visual change is
intended. This source-only delivery contains no generated screenshots.

## Acceptance state

Identity baseline/repeat, deliberate red mutations, candidate equivalence,
host suite, host timing/cycles, Elite/Pro builds, RAM, code size and device PERF:
**NOT_RUN**. No gain, bit-identity, build, RAM or merge-readiness claim is made.
Superpowers TDD/verification and Compound Engineering work/review are used with
the owner's explicit source-only exception: test execution belongs to Orchestrator.

## Static source review

Compound Engineering review completed with correctness, project-standards,
testing, maintainability, performance and adversarial reviewers. One independently
validated gap was corrected: the script now drives active SMC popup entry and
expiry/restoration in both palettes. The review also prompted changing-knob,
active-decay and playback timing cases and a proved cold silent-idle case.
The simplification pass corrected trace-pipe descriptor ownership and alternated
timing samples. These are source review results; compilation and all executed
acceptance evidence remain NOT_RUN.
