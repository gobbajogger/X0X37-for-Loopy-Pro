# F1: Chicago/Miami source candidate

Source-only draft for rows 1–7, 9, 10, 21 and 106. Base:
`build/0.0.23@bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.
Branch: `chat/f1-chicago-miami-controls`. B2 was not found among PRs targeting
this base or branch searches for B2/dropout. No B2 optimization is duplicated.

## What changes for the player

Chicago's FX TYPE selector gains ENSEM, SH DLY, BPM DL, HALL and GT REV after
the original seven choices. All 81 Chicago factory programs become selectable;
the first 55 keep their existing indexes and exact immutable program values.
Miami retains its 30 programs, controls, sync divisions and shape pads.

## Port ledger

Ancestry was checked through GitHub compare before editing. A non-ancestor may
already have equivalent source; the statements below distinguish these cases.

| Named source | Disposition |
| --- | --- |
| `ae0f05116fdccd999c4b8202fd5a1b472fa9de61` | Port compact five-insert owner/header and dedicated test. No new DSP design. Preserve 4096 B arena; reject the non-advancing idle shortcut as described below. |
| `8233297d8b3720eafdf2e77952c849bd1f6343b8` | Twelve-choice cell/integration expectations included through the more complete `fd1af79b3` tests. No duplicate overlapping patch. |
| `c3e1f7f8a4778dea003e9c4433d83d87a770c3ea` | Port the five short cell names and corresponding expectations. |
| `000bd821cb144e1def0470f76a009790a25ce21f` | Port short-name expectations into the dedicated insert test. |
| `fd1af79b3a9c65a0671c92003ee732794f5e23ec` | Port all-81 factory map/count and its exhaustive cell/integration checks. Its alternative larger float-Hall arena/runtime and runtime test are not combined with the selected compact DSP. The compact port is explicitly named in the owner brief and preserves the history allocation. |
| `d8316b0d2aae04b4926979fae827e9018d4e9d70` | Not an ancestor; equivalent centred pitch-bend code already present in base `original_sound_centered`. Do not duplicate. |
| `8d314087a713595d7dbf93e9f9be4f776ee77813` | Not an ancestor; base SMK wheel fixture already wraps `engine_bass_mod`. Do not duplicate. |
| `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21` | Ancestor; this commit documents the rate-pad integration, rather than implementing it. Base UI/HAL/tests contain the actual integration. |
| `11d87db08c0d92f3a5e1e6e729d893b583b3703c` | Ancestor; four Miami shape pads and inert empty pads already present. |
| `9e237cce304dd021792ea80e7426d5f3bbb2a671` | Reference only: waveform/Chicago ADSR rendering already has source in base. No unrelated visual rewrite. |
| `d7f156eca43ccf9ede1d291bc71743b356883e03` | Reference only: per-step sound snapshots are a separate feature, not a gap in the requested pattern binding. No snapshot port. |
| PR #629 (merged) | Retain base preset request/adoption, pattern sound ownership and fade implementation. Cover with existing tests/checklists; do not rewrite it without a demonstrated gap. |
| PR #764/#738 and `archive-missed-0.0.23.md` section 5 | PR #764 head is `000bd821c`, the named compact source used here; #738 is a separate preview/reference lane. The archive document is absent from the verified base tree. No missing archive content is invented. |

The archived `chi_fx2_insert_advance_zero` checks only history silence and returns
success without advancing heads/phase. This would change the next note. Restored
inserts instead refuse the shortcut without mutation and run ordinary DSP. The
new `idle_then_note` case compares PCM and arena state against ordinary processing,
including zero/odd spans and a stereo note. No amplitude threshold clears tails.
The archived parameter mask also wrapped invalid values; the port refuses values
above 127, matching the alternative restore implementation's admission rule.

Test expectation corrections preserve the intended checks. The cell test formerly
expected linear `value/127` even for pitch bend; it now independently checks the
already-required centred mapping, explicit neutral 64 and endpoints, retaining
linear damper behavior and all historical-cell refusals. The existing idle-note
fixture now reads `fx.arena.history`. Its historical reference builder adapts
exactly two `s->history` pointers to that new member path, records the adaptation,
and rejects unexpected historical shapes. No DSP expressions or PCM/state
comparisons are removed. The new insert test checks an independent asymmetric
L-R relationship, disjoint part storage and invalid-edit refusal.
The port's compile-time arena-size bound uses a C99 negative-array-size assertion
instead of `_Static_assert`, preserving the same limit in strict C99 fixtures.

The named compact Hall stores allpass history in Q12 and BPM Delay stores a
half-rate ring. These are explicit properties of the requested source, not a
claim of bit-exact equality to archived float/full-rate effects. The original
seven algorithms retain their existing paths. Fidelity acceptance remains pending.

## Per-row verification and device checklist

Every runtime check below is **NOT_RUN** here. Target `AGENTS.md` prohibits
builds and host tests in Codex, and the owner's brief explicitly delegates them.
Source inspection proves presence only. No RED/GREEN, CPU or RAM pass is claimed.

| Row | Test or concrete device check |
| --- | --- |
| 1 | `original-release-integration`: select both engines in both slots, independent factory adoption and simultaneous Chicago owners. On device, select Chicago/Miami in either part and play both. |
| 2 | `original-idle` standalone differential for original paths; `chicago-fx2-inserts` impulse/tail/reset and idle-then-note; existing AU/golden comparison fixtures. On device compare agreed reference sounds, quiet tails and attacks for each restored insert. |
| 3 | `original-release-ui` visits/edits every agreed nonperformance control, validates native projection, and keeps performance controls read-only in parameter pages. Compare changed sound for each encoder; no Miami ADSR. |
| 4 | `original_wheels_integration_test.sh`, `smk-wheel-mode`: exact physical/MIDI neutral, own-voice pedal/mod routing, held ownership across navigation. Device: bend both ways/release, pedal down/up while switching selected part, verify original voice releases. |
| 5 | New registered `chicago-fx2-inserts`: twelve FX admission/names, impulse/tails/reset, live BPM, transactional idle differential. Integration renders all twelve in both parts. Device: each agreed effect, both edits at endpoints, tempo change, stereo/tail audition. |
| 6 | Newly registered `original-sound-cell`: all 81 raw Chicago programs exactly once, stable old 55 indexes, names, exact untouched floats, all control edits and invalid transactions. Integration selects all 81/30 in both slots. |
| 7 | Existing `bass-presets-live`, `bass-tagged-storage`, `project-stage`, plus `original-release-integration` revision-13 roundtrip. Device: two patterns with different engine/preset/edits, switch/reload; confirm each pattern retains its engine and sound. |
| 9 | `original-build` composition gate checks actual original-engine build routing. No legacy bass303/wobble engine objects are introduced. Device verify engine identity through package/model metadata, not USB name. |
| 10 | `bass-presets-live` exercises pending/cancel/adoption and fade; `original-release-ui` checks/captures names for all restored presets. Device: rapid preset changes during held notes and release tails; listen/record for clicks and check pending name versus active sound. |
| 21 | Existing `freeplay-rate-pads`, `freeplay-live-keys`, and `original-release-ui`: native division order 1/2,1/4,1/8,1/16,1/3,1/6,1/12,1/24; TRI/DOWN/UP/SQR; empty pads 13–16; settings/help and ownership/opt-out negatives. Device check tempo sync and no stray notes. |
| 106 | Device PERF average/peak after cold idle and after released tails, each engine independently, every factory/effect where practical; retain package/model/head and timing window. Must demonstrate <=20% idle per engine. Values below are unmeasured. |

## External runner commands

Use a complete checkout of the final PR head with the pinned V1.2.1 inputs.
Do not execute these commands in Codex. Record actual exits and preserve failures.

```sh
X0X_TEST_JOBS=1 sh tests/run_tests.sh
sh tests/host/original_idle_test.sh build/f1-original-idle-FRESH
BASE_REF=bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2 python3 tests/host/run_chicago_idle_then_note.py build/f1-chicago-idle-note
sh tests/host/original_wheels_integration_test.sh build/f1-original-wheels-FRESH
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
X0X_DEV_PADCAL=0 MODEL=pro ./build.sh --board smk37
```

The standalone idle script requires a fresh output directory. The full host runner
includes the new insert and cell selectors plus extended integration/UI cases.
Do not drop checks or relabel failure as a skip. Required qualification remains
full host 0 fail/0 skip and both native models. No install/flash is authorized.

## Memory and idle CPU receipts

| Measurement | Elite | Pro |
| --- | --- | --- |
| Static free RAM, base reference | 2352 B (owner-provided) | 2352 B (owner-provided) |
| Static free RAM, final candidate | NOT_MEASURED | NOT_MEASURED |
| Pool free, final candidate | NOT_MEASURED | NOT_MEASURED |
| Chicago idle average/peak | NOT_MEASURED | NOT_MEASURED |
| Miami idle average/peak | NOT_MEASURED | NOT_MEASURED |

`engine.c` allocates both compact owners in `X0X_POOL`. The port keeps the history
arena at 4096 B and adds per-owner tempo/edit scalars. This is an intentional
pool cost for live BPM updates, not a measured static-RAM decrease. Verify native
alignment, pool reserve and static RAM on both models. Any static decrease below
2352 B needs a written reason before acceptance.

For CPU: use the exact matching-model package, stopped transport, no held MIDI or
physical keys, fixed UI page, and no USB audio stream. Warm for 5 s, record 30 s of
PERF average/peak, then play/release notes and repeat after tails settle. Repeat
with the other part selecting each engine and with dual Chicago/dual Miami.
Retain noisy/unsettled results separately. Host wall time is not device CPU%.

## Framebuffer evidence

No PNG has been generated in this Codex run. Required outputs on the authorized
runner, all actual host firmware framebuffer dumps at 240x240:

- BEFORE: base `original_release_ui_test.sh` into `build/f1-before-ui` and base
  `original_release_integration_test.sh` into `build/f1-before-integration`.
- AFTER: final-head scripts into `build/f1-after-ui` and `build/f1-after-integration`.
- Retain original seven FX captures as before/after controls. After-state paths:
  `build/f1-after-ui/normal-images/chicago-fx7.png` through `chicago-fx11.png`;
  `chicago-restored-preset55.png` through `chicago-restored-preset80.png`, and
  corresponding `chicago-restored-readout55.png` through `80.png` in the same dir.
- Integration normal/sanitized PNG equality now covers choices 0–11, not only 0–6.
  The old check stopped before the restored choices; the new check preserves all
  old comparisons and adds the five new ones.

Attach those actual PNGs to the PR. Restored choices did not exist in the base;
show its unchanged FX page/preset maximum as the before state. Do not fabricate a
before render for unavailable choices. Keep draft and obtain owner approval
before merge after tests/builds/RAM/CPU/visual evidence is complete.
