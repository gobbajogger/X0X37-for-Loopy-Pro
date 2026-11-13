# 303 navigation cutoff regression

Changing the visible page/part must not change sound. The patch removes wheel
restoration/retargeting caused by navigation; it does not restore on reentry.

## Exact inputs and reproduction

- Owner-installed source: `58e2c5205a02836df678083eaa8db9994b6192e8`.
- Accepted NIGHTLY/base: `08d3164a265123cb4c84e5104642fd93257d63d7` (remote readback confirmed).
- `main_fm1.c`, `ui.c` and `smc_mixer.c` are unchanged between these inputs.
- Independent dependency: PR188 `4baf1160f69d8bbc55ec36ca2dae22c22856f3f4`,
  firmware tree `f000d4d5fcbe5ad3d1cdae8ff16699a990272ef1`, contains PR182
  `e2856159236de7b8e6587f05f139436ac72967a5`. That Decay-after-sustain fix
  is separate; this patch does not edit engine.c, sequencer or DSP.

On either input: select 303A, hold a nonzero mod wheel, play a repeated cutoff
lock of 77 with stored base 30, then select 909 (also through connected SMC).
The production wheel scanner queues T_303/A/CUTOFF=30 at exit. Stored/displayed
base remains 30 while DSP changes from 77. Reentry can queue wheel-derived 73.
Motion's `applied` cache still says 77; identical following locks need not resend.
Changing ramps or knob updates can hide/end the error. Queue -> bass303_set is
observed; this is a source/host reproduction, not a physical listening claim.

## Fix

Pitch and mod gestures independently latch their original 303 until physical
release. Navigation does not restore the previous part or apply the stationary
wheel to a newly selected part. Actual wheel movement/release still sends sound
commands. A stationary wheel does not overwrite a subsequent knob edit or motion.
Project layout, mappings, saved values, motion engine and command queue are unchanged.

Existing intentional held EDIT preview cancellation (note-off/preview restoration)
is a separate gesture-lifetime path and is not changed or certified here.

## Verification

`sh tests/host/smk_wheel_mode_test.sh`: 96 navigation/control pairs PASS on native
macOS Clang. Extracts the actual production wheel function and links real UI,
SMC, engine queue, motion and bass DSP. Both bass parts; stopped live notes and
running notes; neutral/mod/pitch/both wheels; no lanes/repeated locks/changing
motion; connected SMC SELECT and local navigation. Every scan asserts zero
navigation commands and unchanged stored/display reference. Nonzero rendered audio
and DSP parameter traces hash identically to the no-navigation run throughout.
Knob edit with stationary held wheel and mod-A/pitch-B independent releases pass.
Stopped cases with lanes allocated do not claim active stopped EDIT previews.

Exact installed and accepted sources both fail the navigation command assertion
(exit 134), including running repeated-lock and connected SMC controls. Example:
`navigation scan=5 queued target=2 voice=0 param=0 value=30; stored=30 DSP=77`.
The test is registered as `smk-wheel-mode` in tests/run_tests.sh.

Independent reviewer reran all 96 cases, a separate strict warning-free extracted
wheel ownership harness, and installed/accepted negative controls: PASS for patch,
FAIL as expected for both old inputs. Source diff/check review passed. A further
96-case run on combined runtime `1072a2ead7171c2e1c7f1e9471318698894716e1`
(firmware tree equal to final PR188) plus this wheel patch passed.
`smk_fader_pickup_test.sh` and `git diff --check` also pass.

## Remaining gates

Source/host evidence only. Native Elite/Pro build, memory/flash budgets, package
qualification, device audio/ADC/radio timing and owner hardware verification are
NOT_RUN. No device access, flash, GUI, hosted tests, protected merge or binaries.
Existing generated host assets were reused for tests; none are committed.
Native Docker inventory was inaccessible at the local socket; no retry/bypass.
Integrator owns combining this patch with PR188, affected target builds and next
qualification stages. Do not infer device verification or installability from host
passes. No toolchains or private captures were copied or published.
