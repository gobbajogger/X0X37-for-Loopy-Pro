# Musician documentation intake and evidence — 2026-11-14

## Inputs and scope

PR #884 head `97a88b44539110ded82257ebb20b0444ba4b8b8f`, target `lane/026-combined-20261112` at `94197fc839b00aff47b98ca02f3c6396e4a6eabf`; roadmap #887 head `f7eaf9e2d76f7feaf133cb4ad687c54df0eac6ee`. #884 review 5900794894 accepts only the four generated-link corrections, not the whole manual. This successor uses an isolated branch and preserves incumbent ownership. The candidate is not a hardware-qualified release; 0.0.25.1 and candidate behavior differ. No device action or website publication is included.

## Coverage and remaining ownership

- D1 / #78: task-first keyboard and browser guidance, with source-checked state continuity, save, BREAK, SONG, motion and project/sample steps. Changed content needs integration and delivery.
- D2: #885 at `82abc7de807a1faff74c12ceaa268adc517257d2` remains with its README owner; scoped first-install review 5897620005 is reusable. Nightly #129 merged README-only as `18e8278031180854437f077f3afc356f567fad97`; it is not a site-publication receipt. The revised manual link needs the README owner's consumption after its durable destination exists.
- I3: nightly #128 at `a93f26822fd7f3061dab323509086756e68e4d1a` needs wording that distinguishes withdrawn `600b3b45` from `94197fc8` and states exact-candidate boot/restart are unverified and installed hash unknown. Existing Pages publisher owns any allowed publication. #130 publication was denied and is not retried.
- #73: existing app, loader-only, uncertain-restart and manufacturer stock routes are described without an invented recovery gesture or device write.
- #75–#85 and #87: documentation covers scoped connection, browser, install, project/sample, backup and history behavior; it does not change or independently qualify those runtime rows. Main hides the local package chooser; nightly source differs. #83 package identity is not new hardware evidence.
- #86: actual 240×240 host-simulator scenes are provided below. Website imagery, physical keyboard depiction and web integration remain with the existing website owner. Browser/WASM boot and audio are not established by #899.
- #110: independent source and usability reviews and local generated-page checks inform this manual, not full product acceptance.
- #113: published 0.0.25.1 and candidate nightly delivery are distinct from installed-device proof. No installer or catalog code changed.

Dot remains sole ROADMAP writer. Proposed updates: retain D1 / #78 as integration pending until exact revised result is consumed; retain D2's existing README owners and scoped receipts; I3 needs corrected notice and the designated publisher's actual delivery receipt; #86 and #110 remain incomplete pending integrated product and applicable checks. Do not promote source, hardware, or publication state from this documentation alone.

## Simulator provenance

The independent scenario executor reported host build and `scenario-getting_started`, `scenario-patterns`, `scenario-song`, `scenario-slices`, `scenario-keysound`, `scenario-edit_midi_rec_guard` PASS using `X0X_TEST_JOBS=1 X0X_TESTS='host-build scenario-getting_started scenario-patterns scenario-song scenario-slices scenario-keysound scenario-edit_midi_rec_guard' sh tests/run_tests.sh` at firmware candidate `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. `sh tests/host/held_chord_integration_test.sh` reported 21 checks PASS; it exercises internal UI/storage/admission, not an end-to-end physical gesture.

The host simulator at the same source ran `tests/scenarios/manual_pattern5.x0x` (15 assertions) and `tests/scenarios/manual_break.x0x` (10 assertions); the latter checks one local slice-recording case. Inputs use host simulator control names, not the printed legends. A simulator can be invoked as `build/host/x0x_host SCRIPT OUTDIR` after `sh host/build_host.sh`. Host binary SHA-256 for the captures was `7a10a7654917c2262ddc8f80ada04a81288ca080b4435fff0fcf25b89ff20709`.

| File | SHA-256 | Shown state |
|---|---|---|
| `tests/scenarios/manual_pattern5.x0x` | `251aa278092250911e1adb73bb4faaa135721759204e46a1bba4a01348397e6d` | Pattern 5 beat and mixed SONG bar |
| `tests/scenarios/manual_break.x0x` | `5784bb935f86f3f3d5a4a43c0ca7d0e32dc1b9ff58b61dfbdf2325349a26b492` | BREAK pin while stopped and playing |
| `docs/img/manual-song-candidate.png` | `4141e77229d8c2aeb0f721e0f35d575ce0625e0473b30ca4415399095c161f66` | SONG bar 1: 909 P5, Miami P6 |
| `docs/img/manual-break-candidate.png` | `2610bebd8578be9679b3c8e390d310d14ae0b5479e0938a9918451e932df5d4b` | Step 1 slice 5, stopped |
| `docs/img/manual-break-playing-candidate.png` | `45f4a7ed00dfbb91910f7e106abb38e52f5a4edf990a3db9dc89eab504a26ed0` | Same slice, playing |

These are simulated firmware LCD outputs, not physical-device photos, live browser emulator evidence, audio qualification or package boot proof. Independent visual inspection verified the captions against the visible states. No external DAW or physical device was tested. Generated website output is a local shell, not a deployed site; generator/template metadata and empty-catalog fallback are owned by the website owner.
