# B4: recurrent wheel noise and parameter-write attribution

**SOURCE ONLY — TESTS AND BUILDS NOT RUN.** The owner explicitly assigned suite/Elite/Pro execution to the Orchestrator's box. Codex authored this candidate and its tests without executing them. No observed red/green result, measured RAM, device result or installable package is claimed.

Base: `build/0.0.23@bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`, tree `e340d89d180879b8d8a2fea07c56800a2feaac5c`. The isolated branch is `chat/b4-synth-param-ownership-023`.

## Proposed repair

The source contains a route by which a resting mod wheel can acquire Chicago CUTOFF or Miami WOBBLE DEPTH: repeated isolated high ADC samples cumulatively increase `mhi`, even with #766's one-count-per-call limit. Later samples at the unchanged physical rest then normalise above the deadband, publish `engine_bass_mod`, and call `motion_touch`. REC/playback can turn those false gestures into motion lanes.

Endpoint learning now requires two equal filtered readings on adjacent mod ADC publications beyond the current endpoint. A 16-bit sequence from the real SMK ADC producer distinguishes a fresh conversion from repeated foreground polls. A sequence gap resets confirmation; wrap from 65535 to zero is one conversion. A changing IIR tail cannot confirm an extreme; its eventual integer residual is within three counts of the physical rest, inside the 12-count deadband. A sustained endpoint still expands one count per fresh conversion after confirmation. Immediate deliberate wheel output and existing output hysteresis remain. This is a narrow source-based fix, not proof that this is the sole source of the owner's observed symptom. It does not claim to reject every transient pitch/low-mod spike or arbitrary multi-sample noise burst.

Five bounded calibration values (range -80..1103) and the rest counter (maximum 201) use 16-bit storage. Including the prior mod sample and consumed sequence, declared scanner state is 16 bytes instead of 24; the HAL adds a two-byte publication counter. That is six fewer declared bytes before compiler alignment. Actual linked static RAM, flash and pool budgets must be measured on the box; **2352 B free** is the brief's baseline, not a new measurement. Any reduction requires a written reason before acceptance.

Pitch centre 64, engine-specific mod routing, recording/playback and the original gesture's destination are preserved. No display or binding change is intended; mockup/framebuffer paths are N/A. SMC/preset edits in this diff are diagnostic hooks only; no #789 popup implementation is imported.

## Optional host trace

`X0X_PARAM_TRACE=1` is a host compiler definition. It prints `param-write` records to stderr with time, source, target, voice, parameter and value. Normal host and device builds leave it disabled, so arguments, trace strings and trace storage are compiled out. Enabling it on a device deliberately fails compilation: host stderr is not a firmware ISR logger.

Sources: `wheel-pitch`, `wheel-mod`, `panel-knob`, `smc-knob`, `smc-pickup`, `motion`, `motion-restore`, `preset`, `midi`. These mark producer intent or direct motion/preset application; they are not acknowledgments that a queued command was applied. A knob record identifies an accepted logical detent, not proof whether the electrical encoder signal was noise. Trace playback on the box must establish the actual culprit before calling the physical report resolved.

`motion-restore` value -1 means restore the exact stored base; this may be an unrounded factory value. A `preset` record with parameter -1 names the committed bank and parameter -2 its factory index. Pending or cancelled preset requests do not emit adoption records. The ordinary SMC fader record normally targets mixer/FX/master controls; SMC synth knobs have their own label.

## Regression coverage authored

- Existing `perf-wheel-noise`: keeps every previous assertion and adds recurrent isolated high mod samples while REC/playback are enabled.
- New `synth-param-jump`: fresh-process cases for both voices and both engines with REC on/off; recurrent outliers, normal jitter, duplicate foreground polls, missed publications and sequence wrap; the extracted production SMK wheel IIR driven by raw735 spikes and raw615 rest conversions; absence of unintended lane allocation or saved-patch edits; stationary knob authority; deliberate mod movement and offscreen release. Separate fresh processes require genuine sustained endpoints to teach 400..645, giving control 45 at raw 550.
- New `param-write-sources`: real scanner/UI/SMC/motion/MIDI/preset paths, state assertions and required source labels in per-voice/per-bank logs. It uses the normal mixer fader target instead of inventing a synth fader mapping.

Both new selectors are registered in `tests/run_tests.sh` as class G, so their asset generation does not race parallel tests. No old check or selector was weakened or removed. Test-first authoring is recorded in the session, but red/green execution is intentionally deferred.

## Orchestrator box handoff

Use the PR's exact full head, SDK V1.2.1 and the pinned JieLi toolchain. These commands are instructions, **not executed receipts**:

```sh
X0X_TEST_JOBS=1 X0X_TESTS='perf-wheel-noise synth-param-jump param-write-sources perf-wheel-motion smk-wheel-mode chicago-presentation preset-navigation-noop' sh tests/run_tests.sh
sh tests/run_tests.sh
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
X0X_DEV_PADCAL=0 MODEL=pro ./build.sh --board smk37
```

For red proof, use a separate disposable checkout of the candidate, replace only `firmware/src/app/main_fm1.c` with its exact base version from `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`, and run `synth-param-jump`. It must fail on recurrent-noise ownership/lane assertions, not an unrelated compile/setup error. Then use the unchanged candidate for green proof. Repeat the relevant probes with the box's supported sanitizer flags. Preserve all actual failure/NOT_RUN results; report the full head, logs and Elite/Pro free static RAM separately.

No GitHub Actions, merge, publication, device probing/install/recovery or controller change is part of this handoff. Do not touch main, `.github/`, protected device regions or the bootloader. FM-1 targets, Miami ADSR and rows 73/120 are excluded.

## Existing implementation disposition

| Named source | Disposition |
| --- | --- |
| `f62e2ce0496bf6526417c53818002cf9d6643920` (#766) | Already an ancestor of the base; GitHub compare reports ahead 24, behind 0. Retained, not cherry-picked again. This candidate closes the cumulative-outlier gap in that implementation. |
| `8401fc231e9e9774422ff478060da408268cd6da` | Accessible but divergent. Its C_MOD, `engine_bass_mod`, `motion_touch` and performance-lane paths are already present in the base and retained. No wholesale cherry-pick or claim of identical ancestry. |
| `d8316b0d2`, `8d314087a`, `restore-features` | Connected GitHub commit/compare reads returned Not Found, commit searches had no matches and the branch was absent. No port claimed. Base already contains exact bend midpoint64 and `engine_bass_mod`; those existing implementations are preserved. No behavior from the brief was deliberately dropped. |
| #789 / A9 | Existing SMC popup ownership remains separate. Only write-source hooks are added to shared UI/SMC paths. No popup/navigation repair or other brief's source is duplicated. |

Independent review and remote source-delivery receipts are recorded separately. Suite, Elite, Pro, RAM and hardware remain **NOT_RUN** until the Orchestrator provides exact-candidate evidence.
