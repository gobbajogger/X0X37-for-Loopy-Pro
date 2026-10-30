# Remaining DnBOX feature intake and proof contract

Source: 29 October 2026 DnBOX 122-item feature register. Original implementation baseline: `nightly` at `66073c68dd80d51a7053432c2c161a8b720a1341`. Integrated target: `wip/one-build` at `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`, descendant of owner-specified `094bb1a55c159aefb0e2bfa39790a1b4b74cee40`. Work lane: `chat/remaining-features`. This is an implementation queue, **not a claim of completion**.

## Scope and exact constraints

The 25 items below remain with this lane after the 22-item Orchestrator owner handoff. They are not assigned to the concurrent `smc-port`, `break-feat`, `mixer-ux`, `web-site`, `ui-theme`, `web-editor`, and `#684` engine lanes, or to already-merged work. Do not overlap those owners' changed paths without integration coordination. Omit items 29, 55, 116, 117, 120 and 122. The Orchestrator owns 4, 7, 8, 11–15, 17, 18, 21, 22, 43, 47–50, 52, 54, 63, 110, 114; do not edit their implementations in this lane. The only synth engines are Chicago and Miami. **No migration; only project revision 13 loads in the final target**. Do not bring back tagged legacy opcode 75, a previous sound layout, private `#684` headers, binary SDKs or firmware packages. Use host testing only for this lane. Do not promote `nightly` to production, publish an installer, or flash a device in this lane.

| Items | Work to close | Required independent acceptance |
| --- | --- | --- |
| **23** | **Preserve distinct 909 and 808 model identity** | **`tests/host/drum_kit_identity_test.*` committed in this lane guards 11 voices per kit, voice numbering, independent patterns, accent and sound storage. The browser's distinct 11-voice layouts remain for the `web-editor` owner to validate.** |
| 30, 38 | Readable eight-strip SMC page; second Select opens individual drum mixer | Host UI render and MIDI feedback tests, with SMC owner coordination before shared edits |
| 57–62, 64–66, 69 | Pattern overview, readable velocity/steps, accurate wave/FX envelopes, knob mapping, navigation, grouped settings and activity-based animation | Approved mockup/review first; production framebuffer snapshots and host interaction tests. Never revive the boot player |
| 73, 74 | Recovery loader concept and SMK37 dead-pad diagnosis | Read-only source/host recovery-state checks; no flash. Hardware pad-scan failure cannot be verified in simulation |
| 76, 83, 86, 88 | Song site transfer, correct bundled controller, real UI screenshots and visual site variants | Coordinate `web-site` first; browser/host tests only, no publication or controller binary commits |
| 112, 113 | Actual Elite/Pro packages and an installable owner-accessible build | **Out of scope for host-only execution**: these require separate native/package QA and publication gates. Never call source-only PASS a delivered installer |
| 115 | Independent exact-input validation after composition | Re-check *final merged tree* + changed compiled inputs and pass/fail of required host selectors; historical proof is not transferable by assumption |
| 118 | Keep main/production separate from experimental NIGHTLY | Preserve model-specific device acceptance and owner approval; no production write in this lane |
| 119 | Shared community song library | Explicitly deferred. Do not implement as a prerequisite for individual project import |
| **121** | **Evaluate official M-VAVE source and workbench** | **Assessment below; source/reuse licensed case-by-case. No binary/bootloader adoption or physical-sensor claim** |

## Owner handoff (29 October 2026, latest owner instruction)

The Orchestrator alone owns: **4, 7, 8, 11, 12, 13, 14, 15, 17, 18, 21, 22, 43, 47, 48, 49, 50, 52, 54, 63, 110, 114**. These are not deliverables of this lane, and no new patch here should modify their functionality.

Our exact 25-item list is: **23, 30, 38, 57, 58, 59, 60, 61, 62, 64, 65, 66, 69, 73, 74, 76, 83, 86, 88, 112, 113, 115, 118, 119, 121**.

Priority: submit standalone host regression and scoped evidence for **23**, establish reusable exact-input evidence for **115**, and preserve source/licensing assessment for **121**. Then implement the other in-scope code features against their current owners' disjoint paths. Visual changes follow mockup → independent review → implementation → actual UI validation. Items 112/113 and deferred 119 cannot be claimed finished merely from host tests; item 118 is a protected release boundary, not authorization to publish.

## First-stage evidence: #23, #115, #121, plus existing #30/#38/#74 audits

At the pinned `nightly` source, a read-only check verified both drum enums have exactly 11 differently ordered voices and distinct kits. `firmware/src/seq/pattern.h` defines two independent drum parts; `firmware/src/app/engine.h` has distinct `T_909` and `T_808` pot storage; `engine.c` owns separate DSP instances and calls `drum909_trigger` and `drum808_trigger` separately. Seven structural source checks passed on the pinned source; these were **not** a current-head compile or DSP/audio quality result.

The committed standalone host test verifies 11 voices and original mapping for each kit, separate step/accent rows, separate sound pots and lossless pattern copying. It compiles and passes with GCC and Clang against the locally available older source snapshot `f58d62f0b622b9c5792bda81793e0031d8be8747`. This is **historical host coverage only**; the new test must run on the exact current candidate before #23 is host-qualified. The test and receipt helper were delivered through the connected GitHub account on the isolated lane (`4710774f8b42adb8a7c4905e4b73d0a79de950f5`); the current-head host run remains NOT_RUN here. The browser/editor half of #23 remains owned by the parallel editor lane. The actual new host test does not change firmware bytes or the project format.

### #121 official M-VAVE open workbench, bounded evaluation

The upstream `m-vave/smk37_open` GitHub repository identifies itself as the official **SMK37 OPEN V01** Pro/Elite source with a matching development kit and browser workbench, as of 30 October 2026. The public README documents separate Pro and Elite workbenches, eight tracks, the source/browser simulator, project and sample management, MIDI roles, recovery precautions and model confirmation. Project firmware uses an entirely different track/engine and storage architecture. **Do not import that firmware or convert DnBOX to its project format.**

Potentially reusable **design ideas**, subject to implementing them against existing DnBOX components: device-model picker with physical model confirmation, browser-versus-device state labels, explicit sample/project backups, distinct parameter ownership for live and recorded tracks, reproducible offline workbench tests and separation of simulator behavior from physical MIDI/pad inputs. These are candidates, **not yet implemented or independently benchmarked improvements**.

Source: <https://github.com/m-vave/smk37_open> (README and `LICENSE.md`, `THIRD_PARTY.md`); official workbench: <https://open.m-vave.com/smk37-workbench>. The upstream SDK includes proprietary binary libraries, model-specific recovery firmware and separate redistribution conditions. Do not vendor those bytes, copy SDK headers, copy recovery images or bypass DnBOX's restricted-source boundary. A simulator run proves UI/application logic only, not real pad sensors, audio budgets, USB hardware or hardware-flash safety. This assessment completes #121's **investigation stage**, not adoption or target validation.

## Existing #30/#38/#74 functionality, no competing patches

Read at the pinned source, **not** validated as one final integrated version:

- **#30:** `firmware/src/app/smc_mixer.c` already draws eight physical strips. `tests/host/smc_ui_test.c` includes physical geometry, touched-lane selection, per-kit titles, lock/bank and screenshot assertions. Do not create another SMC renderer without a demonstrated current failure.
- **#38:** `firmware/src/app/smc_mixer.c` already handles repeated Select on a drum kit to open its mixer, and a repeated selected drum strip to return to the parts mixer. `tests/host/smc_select_mode_test.c` and `smc_drum_rs_test.c` contain relevant navigation and selection assertions. Acceptance requires exact-current-source rerun and screen review, not reimplementing this behavior.
- **#74:** `firmware/src/app/main_fm1.c` implements read-only INLOG arg 6 (raw/base/peak, status, pressure, scan epoch and mux values). `tools/inlog_fmt.py` decodes strict 521-byte snapshots, and `tools/ble_autotest.py --pad-inputs` requests the snapshot without a write. Existing `tests/smk_pad_inputs_test.py` exercises the real snapshot function with 528 copy collisions, 1563 injected serialization interrupts, strict decoding and fail-closed CLI behavior. A duplicate host test was intentionally discarded. Physical sensor cause is still unknown and requires authorized device-side observation, not simulation or threshold edits.

No production code, bootloader, firmware bytes or private headers were changed in this lane by these inspections.

## Host-only exact-input receipt for item 115

`tools/remaining_feature_proof.py` now runs a pinned plan, verifies committed input blobs against the checked-out Git tree, captures real command exit codes and SHA-256 log digests, compiler/runtime binary hashes and selected environment variables, and refuses dirty source or changed receipts. The negative selftest deliberately checks corrupted logs, changed source, failed commands and unsafe input paths. The initial plan is `docs/remaining-feature-host-plan.json`. On an exact new source commit, run:

```sh
python3 tools/remaining_feature_proof.py run --repo . --plan docs/remaining-feature-host-plan.json --out build/host/remaining-feature-proof
python3 tools/remaining_feature_proof.py verify --repo . --out build/host/remaining-feature-proof
```

This scope is #23's standalone host test plus the proof helper's own selftest, **not** complete independent review of the remaining 24 items. Generated drum tables are regenerated from the three tracked WAV inputs and the tracked generator in an isolated host output path. The receipt must be regenerated and independently reviewed on the actual candidate, not reused from the historical snapshot.

## Next accountable integration step

The host regression, scoped receipt helper and tests were committed to the isolated `chat/remaining-features` branch using the connected GitHub account default author. Run `sh tests/host/drum_kit_identity_test.sh` on the candidate's complete exact-source checkout (GCC/Clang if available). Independent QA must review that exact commit/tree and each further affected feature. A PR to `nightly` stays open until its required host proofs and feature scope are complete. Use the connected GitHub account default author; do not set a custom commit author. No main merge, firmware flash, native package, gh-pages or production promotion follows from this document.

## HOME source candidate: #57 and #58

The existing 8-column visual footprint is retained. `home_pattern_glyph.h` now summarizes all saved steps, rather than presenting steps 17–64 as blank, and uses the strongest real event velocity per column. A bounded host regression passed on GCC and Clang UBSan with historical header inputs; current-HEAD HOME framebuffer validation is **NOT_RUN**, so these items remain pending visual acceptance. See `docs/notes/remaining-home-overview-evidence.md`.

## Installer song/project entry: #76

The existing installer now links directly to `/projects/`, which already exports and imports the versioned `.x0xproject` file containing song arrangements, patterns and sounds. The project page retains its existing preflight, sample identity and recovery behavior; no second song format or transport was introduced. English and Japanese link/help strings are provided. The scoped Node host test passed against the feature fragment and existing historical project page; current-HEAD browser navigation and exact generated-site rendering are **NOT_RUN**. This is project-inclusive song transfer, not a new song-only file format.

## Integration onto current one-build

The existing PR #694 branch cannot be history-rewritten without force-push. The integration commit uses two parents (the prior #694 head and the current one-build head), preserving a fast-forward update of the feature branch. It is a source-level port and merge, **not a literal history rebase**. Retarget PR #694 to `wip/one-build`; do not merge it automatically.

The HOME renderer preserves the newer unified UI geometry and only ports complete-pattern/velocity glyph semantics. Prior `nightly` test receipts refer to older source and must not be reported as exact integrated-head PASS. This task requires local host checks and an explicit source closure receipt before claiming final integration proof.
