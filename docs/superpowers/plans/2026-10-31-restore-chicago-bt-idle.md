# Chicago FX, Bluetooth and idle UI restoration plan

> For agentic workers: use superpowers:subagent-driven-development task by task.
> The root integrator owns authoritative checks and all canonical commits.

**Goal:** Restore the requested historical behavior on one feature branch and one
draft PR, after approval of real-framebuffer UI proposals.

**Architecture:** Preserve the integration branch's native Chicago/Miami engines,
existing controls and persisted indices. Port bounded donor helpers for missing
inserts and UI behavior; keep one selected per-part FX owner. No whole donor-base
replacement or new DSP algorithms.

**Tech stack:** C99, real host UI/framebuffer, Python build/packaging tools, genuine
JieLi clang 4.0.1 and pinned AC79 SDK V1.2.1.

**Spec:** `../specs/2026-10-31-restore-chicago-bt-idle-design.md`.

## Global constraints

- Base `e2c33498d33c425287f1df077b8ac6cc89a6e8aa`, branch
  `feat/restore-chicago-fx-bt-idle-extras`, PR base `integration/all-in-one`.
- Owner approval of BEFORE | AFTER framebuffer proposals before production UI edits.
- Author and committer `gobbajogger <255244738+gobbajogger@users.noreply.github.com>`.
- No `.github`/Actions, main/nightly writes, PAD DIAGNOSTICS, retired engine restore,
  protected device-region or bootloader changes, binaries/captures/SDK/toolchain commits.
- Existing private headers are allowed source. SDK V1.2.1; only Elite/Pro DEV.
- Preserve revision-13 existing choice/factory indices and unrelated functionality.
- The missing BT SAVE PENDING donor is a pending dependency, not permission to invent.

## Review focus

- Existing projects must keep the same FX selection and factory sound after reload.
- Switching inserts, tempo or control values must not expose stale tails or cross-part state.
- Long/identical/disconnected BLE peer names must fit the native row without overwriting memory.
- Held-step, motion and touched-knob identity must select the correct schematic and waveform.
- CPU values above 255 must display truthfully without altering measurement or audio timing.

## Task 1: Approve source-backed UI proposals

**Files:** design document above; scratch-only renderer fixtures and native PNGs.

- Characterize the exact base with existing peer-names and save-note regressions.
- Render native framebuffer BEFORE and AFTER proposals using donor source for
  names, waveform, schematic, CPU and the appended FX choices. Do not edit captures.
- Obtain independent review of semantics, clipping, edge states and feasibility.
- Deliver captures outside Git; record their source pins, model flags and hashes.
- Wait for owner approval. Approval is required by the current user instruction.

## Task 2: Restore Chicago insert availability

**Files:** `firmware/src/app/{engine.c,original_sound.c,original_sound.h}`;
existing `firmware/src/app/chicago_*.h` helpers and focused actual-engine tests.

**Interfaces:** current original runtime float L/R output; current FX choices 0..6;
historical compact selected-state helpers; current internal/external tempo source.

- Establish unchanged PCM/control/state characterization for all existing choices
  and factories, including save/reload and both Chicago parts.
- Add failing current-engine cases for appended Ensemble/GateRev/Hall/BPM/Short
  choices, independent histories, endpoint controls, tempo updates and A/B/A reset.
- Port the five existing compact helpers and dispatch/lifecycle from #611/#677.
  Preserve the unaffected native path and existing persisted indices. Do not bring
  back bass303 or depend on historical bass303-generated test signals.
- Verify actual-engine output against the recovered helper for the added effects,
  while existing choices remain unchanged. Check motion and same-frame events.
- Run focused normal/sanitized checks and measure dual-part pool/code/RAM budgets.
- Root reviews and commits this unit only after its authoritative checks pass.

## Task 3: Restore connected-device name UI

**Files:** `firmware/src/app/{ui.c,plat.h}`, `host/x0x_host.c`;
existing `tests/host/smc_peer_names_test.{c,sh}`.

**Interfaces:** existing `plat_ble_peer_names(char names[2][21])`, R_BTNAME and
the host implementation guard. No controller/link-configuration changes.

- Run the existing test as the negative baseline and retain its actual errors.
- Port only the #687 helper/hook/header declaration/host stub guard; preserve
  enough local buffer space for two maximum-length names and separator.
- Run its actual BLE aggregation and real UI cases in explicit Elite/Pro host
  configurations; check one/two peers, identical/long/unnamed names and disconnect.
- Verify unchanged row/navigation behavior and normal/sanitized framebuffer equality.
- Root reviews and commits the bounded unit.

## Task 4: Identify and restore BT SAVE PENDING

**Files:** to be bounded by the recovered source; no speculative implementation.

- Resolve whether the owner means the physical BT/Save queued project save or
  a separate Bluetooth-settings persistence retry.
- Reuse existing #632 behavior unchanged if that is the requested already-present
  feature; run `save_note_quiet_test` and record the exact coverage.
- Otherwise locate the exact original source before editing. Port its existing
  pending/retry behavior, prove transient failure retains the request, and prove
  a later quiet retry persists once without touching pairing/device regions.
- Render any changed feedback using real framebuffer and obtain approval if it
  differs from the previously approved proposal. Keep the subtask blocked if no
  lawful exact source is found.

## Task 5: Restore idle-glyph extras

**Files:** `firmware/src/app/ui.c`, current `ui_original_controls.h` or a bounded
projection helper; existing original-release UI/ADSR tests and donor oracle cases.

**Interfaces:** current original program/native controls; current locks/motion and
matrix touch identity; existing CPU percentage getters and PERF state.

- Establish native-projection and CPU overflow failures before production changes.
- Port #681 cached 29-point waveform math and relevant-preview invalidation.
  Validate against the original oscillator evaluator across admitted factories,
  equal-byte previews, locks/motion and malformed input; preserve project state.
- Port only the original-engine touch schematic; preserve the newer inline ADSR.
  Test touched-stage/held-step ownership, timeout, band bounds and native labels.
- Port saturating 16-bit PERF fields. Test 120/255/256/300/511/65535/65536/UINT32_MAX
  and preserve timing/EMA/measurement code unchanged.
- Run affected UI and sanitizer checks; compare actual candidate framebuffer
  against the approved proposal, including long labels and both engines/parts.
- Root reviews and commits the unit; exclude every pad diagnostics/calibration hunk.

## Task 6: Qualify and update the single draft PR

- Freeze final source and perform Compound Engineering review with independent
  reviewers. Resolve in-scope findings, then re-run only affected checks.
- Run the full applicable SMK-only host suite and genuine V1.2.1 Elite and Pro DEV
  builds, controller/model integrity and code/RAM/pool checks. Never call a runner
  that silently builds or tests FM-1. Required failures/skips prevent qualification.
- Report any base failures separately; baseline/proposal results do not qualify
  the restoration candidate. Do not claim hardware validation or release readiness.
- Inspect exact staged paths and authors; commit only approved source/docs/tests.
  Push only new changes, verify remote parent/tree/head and reuse the same draft PR.
- Update FEATURE-REGISTRY and post an attributed status comment with exact results,
  remaining blockers and next action. Never merge to main/nightly or enable auto-merge.
