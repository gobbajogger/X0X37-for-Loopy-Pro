# Project actions and portable files: development audit

This review covers source
`68e1117262aba958f96b6eb65cb3bfb1b3cc94ce`, compared with remote main
`f8420eb3749c03e4ba2e24a0fb6ba7b97f627f97`. Host results below are completed
unless explicitly qualified. Elite and Pro are **DEV CHECKED** in
`projects-validation/elite-checked` and `projects-validation/pro-checked`.
Both artifact records and the focused package/controller checks passed. This
audit records development validation; publication provenance is recorded separately.

## Implementation

- **On-device project actions:** guarded Save, Load Saved and Reset Project.
  Load asks for confirmation when unsaved changes exist; Reset requires
  confirmation. Both replace the complete project state and start fresh
  runtime/undo state. Defaults include sounds, settings, patterns, song and
  motion data. These actions preserve sample slots and device calibration.
- **Staged validation and adoption:** project header/version/CRC, semantic fields,
  stored-object completeness and pattern capacity are checked before adopting a
  staged project. Rejection preserves the live project, settings, sample bytes
  and memory gauge. Capture/export does not mutate the live state.
- **Runtime exclusion:** replacement stops old voices, releases notes with the
  old routing, resets SMC/SMK queues and pickup state, and consumes held controls
  and MIDI received during the transaction. Queued update/reboot requests cannot
  cross a project transfer. Shared project/save/performance scratch remains
  guarded; queued or explicit save is allowed in performance phase 0 before
  `perf_build`, and blocked through phases 1–3 and restoration.
- **Portable companion:** `.x0xproject` files contain the project plus, by
  default, the original active sample images and complete 512-byte headers.
  Slot ordering and empty slots are preserved; there is no resampling,
  re-encoding or slot remapping. Project-only imports require matching sample
  identities and issue no sample erase/write commands. Portable import validates
  before writing, requires a downloaded recovery backup for destructive slot
  changes, verifies exact readback and attempts rollback on failure. An
  unverified rollback retains the firmware recovery hold across timeout and
  browser reconnection. The page is included in the generated `projects/` route.
- **Flash space:** lossless glyph compression preserves the existing font metrics
  and rendered pixels. Default Barlow bitmap storage is 16,532 bytes smaller;
  Terminus is 21,060 bytes smaller. These are asset savings, not final linked
  application budgets. Features, sample quality and memory limits were retained.

Primary source: `firmware/src/app/{project,editor,ui,engine,main_fm1,smc_mixer}.c`,
`firmware/src/gfx.c`, `tools/gen_font.py`, `web/x0x_projects.{js,html}` and
`docs/PROJECT_FILES.md`.

## Review findings resolved

Independent storage, runtime, companion and font review led to concrete fixes:
valid conditional probability values remain accepted while derived TAPE values
are bounded; interrupted sample changes cannot release the runtime before
verified restoration; sample metadata is checked against original image headers
even when both JSON copies and CRCs are forged consistently; and stale
update/control events are discarded. The physical-control extraction fixtures
now include the production transaction guards and assert held-input behavior.
Menu scenarios were aligned with the added Load Saved row and fresh undo history
after Reset. The final save-window correction preserves the original phase-0
save opportunity without saving borrowed performance scratch or temporary
settings.

The font decoder received independent source review and a production-C pixel
comparison. The companion's forged-metadata regression failed before its fix and
passed afterward. Review did not establish physical-device behavior.

## Completed host evidence

Evidence paths are relative to the task workspace. The bundle index supplies
their relocation.

| Coverage | Verified result | Evidence |
| --- | --- | --- |
| Storage, defaults, corrupt/truncated load, semantic/capacity rejection, nonmutation and sample preservation | **24,446 checks, zero failures**; production storage code with simulated flash | `build-setup-evidence/project-stage-final-sanitized.log` |
| Staging and protocol sanitizers | Focused **ASan + UBSan** runs passed with `ASAN_OPTIONS=detect_leaks=0`; address/undefined instrumentation remained enabled | `build-setup-evidence/project-stage-final-sanitized.log`, `build-setup-evidence/project-protocol-final-sanitized.log`; runners accept `SAN_FLAGS` |
| Protocol | Exact chunk offsets/replay, CRC/capacity, confirmation, timeout/recovery and full sample headers passed | `build-setup-evidence/project-protocol-final-sanitized.log`; `project-protocol` |
| Legacy migration | Format-2 pattern/arrangement expansion passed, including overlapping storage | `projects-validation/elite-final/check-0.log`; `project-303-stream` |
| Runtime, SMC queues and update exclusion | Reset/load/cancel/rejection, sample preservation, queued inputs, save-window and update guards passed against the final UI change | `build-setup-evidence/project-runtime-perf-window.log`; `projects-validation/elite-checked/check-1.log` |
| SMK input code on the host | Clip mode/pad edges, keybed/knob hold, pending save suppression, resumed fresh input, fader pickup and calibration dispatch passed | `smk-clip-isolation`, `smk-clip-knob`, `smk-fader-pickup`, `smk-padcal-wiring`; `projects-validation/elite-final/check-0.log`, `projects-validation/elite-checked/check-1.log` |
| Font storage/rendering | **5,204 rendered cases per font set** retain identical pixels and metrics; encoder run/literal boundary cases also passed | `build-setup-evidence/font-packing.log`; final `projects-validation/elite-checked/check-3.log` |
| Portable files and page actions | **29 core + 7 page tests** passed: corruption, exact sample identity/header preservation, collisions, failed acknowledgments, rollback, recovery hold/reconnect and confirmations | `web/test_projects.mjs`, `web/test_projects_page.mjs`; final `projects-validation/elite-checked/check-4.log` |
| Menu/scenario consumers | All seven corrected scenarios (`autosave`, `perf`, `perfsave`, `undo`, `guard`, `tour`, `arrows`) and `perf-scratch` passed on the integrated source | `projects-validation/elite-checked/check-2.log` |

### Reuse of unchanged inputs

The final Elite plan reuses only `project-303-stream`, `project-protocol`,
`project-stage`, `smk-padcal-wiring` and `smk-fader-pickup` from the successful
check at `b104b7b5b774c79d2ace8e389066ddfdf72114e9`. The verifier pins that
source tree, the successful log and environment-script SHA-256 values, permits
only the exact reviewed non-consumer changes, and compares the UI function bodies
consumed by calibration wiring. Changed runtime/clip consumers and corrected
scenarios rerun. `projects-validation/elite-checked/check-0.log` records the
successful reuse verification. The plan and dependencies are
`build-setup-evidence/projects-elite-plan.json`,
`build-setup-evidence/reused-project-checks.json` and
`build-setup-evidence/verify-reused-project-checks.py`.

Pro reuses the model-independent host results with the same non-Markdown source
fingerprint. Both explicitly requested models have their own target/controller
builds, budget checks and artifact records. Their package identities and final
artifact verification are recorded below.

## Final source and package record

| Field | Elite | Pro |
| --- | --- | --- |
| Validated source commit | `68e1117262aba958f96b6eb65cb3bfb1b3cc94ce` | Same |
| Non-Markdown source fingerprint | `1814e83f0d9d4f77af715c79acf44b5627de18defc2da6cd046ea70fdc370a7e` | Same |
| Configuration | Development; `X0X_DEV_PADCAL=1` | Development; `X0X_DEV_PADCAL=1` |
| Application bytes / XIP free | 544,240 / 8,432 | 544,240 / 8,432 |
| Static RAM used / free | 97,780 / 524 | 97,780 / 524 |
| Pool used / spare | 335,740 / 8,324 | 335,740 / 8,324 |
| Loader bytes | 7,280 | 7,276 |
| `.fwsc` filename / bytes | `x0x-smk37-elite.fwsc` / 651,396 | `x0x-smk37.fwsc` / 651,392 |
| `.fwsc` SHA-256 | `a62f2f22275029a197bad09a87b2c267fa91476bf8758c9883f5c8160609a084` | `60fc608868dc9b8893ee26ed292aca7ccec9f8f9b7f322dc76e9eb5a053cb6b3` |
| Controller image bytes / SHA-256 | 65,492 / `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | Same |
| `record.json` location | `projects-validation/elite-checked/record.json` | `projects-validation/pro-checked/record.json` |
| `record.json` SHA-256 | `06e703d33ee7897984ea3ce7a3707d65abe5f5a76ea845e709a641c6a6fd9367` | `70491f3cd8fabb369b7614b63ca3707445e5d30afe2970e35cec9e9501e76fb7` |
| Exact package identity/layout check | **PASS:** `SMK-37 Elite_900`, 20 blocks, CRCs/layout | **PASS:** `SMK-37 Pro_900`, CRCs/layout |
| `dev_validate verify` | **PASS** (integrator report; preserve final verification log) | **PASS** (integrator report; preserve final verification log) |
| Controller structural check | **PASS** | **PASS** |

Unchanged guards: application at most 552,672 bytes before BLE insertion;
`.data + .bss` at most 98,304 bytes; 344,064-byte pool with at least 8,192 bytes
spare; existing RAM-call, soft-double, settings and HAL checks. Retain the exact
recorded artifacts. Both models have 132 bytes above the required pool spare and 524
bytes static RAM headroom. An embedded-controller package has a different hash and must
have separate provenance. No release configuration or full release gate is part
of this work.

## Limits

No firmware was installed, and no physical MIDI, device flash-write/readback,
power-loss or unplug/reconnect recovery experiment was run. Flash faults and
transport failures above are host simulations. Persistent multi-object saves
and portable sample replacement are **not power-failure atomic**.

No Chromium visual/browser smoke was available. Page tests execute the actual
page code with a controlled DOM and device service; font tests compare production
C pixels. Neither establishes browser rendering or physical display behavior.

The broader runtime UBSan run reported the pre-existing negative left shift at
`firmware/src/dsp/fastmath.h:57` for values `-1` and `-2`
(`runtime-lane/build/host/project-runtime-sanitized-run.log`). Do not describe that
run as sanitizer-clean. The focused storage/protocol sanitizer passes above are
separate evidence. A later Markdown-only audit commit must retain the verified
non-Markdown source fingerprint; reverify both artifact records against that
candidate before publication.
