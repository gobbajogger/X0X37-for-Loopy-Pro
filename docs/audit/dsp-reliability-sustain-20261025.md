# 303 DSP reliability: live Decay must preserve completed ADSR sustain

## Delivered finding and repair

On NIGHTLY `08d3164a265123cb4c84e5104642fd93257d63d7`, select filter
ADSR, Attack 0, Decay 0 and Sustain 64. Hold MIDI note 45 for one second,
then increase Decay to 127. The envelope rises from `0.503937006` to
`0.950393677` and stays there beyond four seconds. The filter opens despite
the unchanged sustain selection. This is a new defect in the existing ADSR
feature, separate from repeated FEnv mode writes (PR99/BF-016).

`filter_adsr()` recalculated decay in sustain stage 3, using the frozen elapsed
counter with the new decay coefficient. The repair calculates decay only in
stage 2 and follows `fsustain` in stage 3. Attack, gate-off release, stock 303
envelope, amp/accent envelopes and slide triggering keep their existing paths.
No voice struct or persistent layout changes. Target code size remains unmeasured.

Source patch: `650d632dfdb4756e7e75b8f334b6ad27e4245862`; firmware tree
`1564ea6433c4d9ae30b0356e23e5977a399d3c1f`.

## Reproduction and observable audio

The new production-linked `bass303_sustain_test.c` renders two identical voices
through completed sustain, then changes Decay in only one. No mock or rewritten
DSP is used. A completed decay must produce the same sustained audio regardless
of its time setting until the next trigger.

Baseline: all four ADSR combinations (normal/lite, accent off/on) differ in
every one of 88,200 compared samples. Difference RMS is 0.03058–0.03104 at
this preset, with envelope `0.950393677`. The same four stock-envelope controls
are identical. Baseline compile exits 0; behavioral test exits 1, as expected.

Fixed: all eight combinations have zero differing samples and ADSR sustain
stays `0.503937006`. Live Sustain follows the selected level; release settles
to silence; retrigger uses the new longer decay; slide does not retrigger ADSR.
Registered `bass303-sustain` and existing `bass303_lite` selectors pass.
OS 1/2/4 × control interval 1/4 pass in optimized and ASan+UBSan builds.

A separate read-only QA agent independently reviewed the diff, reproduced
baseline failure and ran the latest registered regression and an additional
180-transition production-linked harness. Attack, live active decay, sustain
0/64/127, sustain changes, release, retrigger and slide passed, including
ASan+UBSan. This is scoped host QA, not target or whole-project qualification.

## Matched audio and timing evidence

`bass303_render_probe.c` renders 24 matched normal/lite × stock/ADSR ×
drive × waveform scenarios, including velocity, accent, slide, release and
retrigger. It does not change Decay during sustain, so it checks preserved
behavior. Baseline and fixed builds render 3,175,200 finite samples each;
all five alternating pairs produce the identical 12,700,800 output bytes:
SHA256 `4703fa9949705a7c24dc8958c3d1b71157b632523bef2495833289cc4e4f3f70`.

Same compiler/input/host: baseline median CPU time 0.107997 s
(0.098772–0.118741), fixed median 0.102839 s (0.097989–0.111405).
Ranges overlap. This measures host voice rendering plus float-file I/O;
**no speedup, device CPU load, XRUN reduction or whole-renderer saving is claimed**.

Numerical probes on baseline and repeated against the patch found no nonfinite
samples across 1,344 corner presets × 51,200 samples and 200,000 deterministic
variable-size live blocks. Optimized and ASan+UBSan runs passed. The corner
probe's pre-master peak is 50.5575; finite does not mean output is unclipped.
This does not qualify every parameter sequence or the master limiter.
Split-block audio differences are consistent with the documented control-rate
approximation; no new defect or block-invariance claim is made.

## Actual path and prior-work reconciliation

- Input: keybed preview queues `engine_bass_on_velocity`; USB/BLE/TRS MIDI
  routes through `midi_in`. Both reach production `bass303_note_on_velocity`.
- Sequence: `fire_bass` chooses gate, pitch, velocity, accent and slide;
  `s_bon` applies probability and triggers the same voice. Rest/off reaches
  `bass303_note_off`. `seq_next` boundaries split the audio render.
- Voice: a nonlegato note triggers amp/filter envelopes; overlapping slide
  retains envelope stage and slews pitch. ADSR affects the filter only.
  The repair is in that common voice path, so live and sequenced notes use it.
- Sound pots persist in the existing project sound mirror. Project application
  reinitializes the engine and reapplies sound after the existing staged-load
  process. The patch changes neither storage nor reset behavior.
- Existing whole-host `step303`, `usbnotes`, `blenotes`, `extclock`, `tb3po`,
  `probcond`, `clkmove` and `song` scenarios pass after the patch. They exercise
  production engine/sequencer/audio, but do not prove the full outstanding
  transport/persistence feature requests or every tempo transition.

Main observed at `c2ca0c1384390aa985b7df83b73dfe9bef40d7c9`; both main and
NIGHTLY carry the same baseline bass303 blob `379981bf...` for this defect.
The request ledger at `ce8bfd4a40179590485ecf35e6bb0be2d652e602` marks
LX-REQ-008/009 combined lifecycle/probability QA and LX-REQ-023 whole-renderer
performance outcomes unfinished. Source presence is not delivered-feature proof.

Reused rather than reimplemented: PR99 repeated FEnv, PR139 probability/orphan
TIE, PR140 RANDOM logical bars/swing/clock handoff, PR152 TB-3PO accent,
PR159 RANDOM TIE preview, PR171 persistence reconciliation. PR174 is a separate
SMC ingress lane. Known earlier queue-release and cross-source note ownership
findings are not presented as discoveries here. No changes to sequencer duration,
SMC/UI, project persistence, website or alternative synth design.

## Reproduce, limits and integration handoff

From the repository root:

```sh
X0X_TEST_JOBS=1 sh tests/run_tests.sh bass303-sustain bass303_lite
cc -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Wdouble-promotion -Werror \
  -Ifirmware/src/dsp tests/host/bass303_render_probe.c \
  firmware/src/dsp/bass303.c -lm -o /tmp/bass303-probe
/tmp/bass303-probe /tmp/bass303-probe.f32
```

For baseline controls, extract the baseline `firmware/src/dsp/bass303.c` to a
separate file and link the same tests/probe against it. Numeric and independent
transition probe sources, exact commands, exits, hashes and timing rows are in
`evidence/dsp-reliability-20261025/receipt.json` and its sibling files. No audio
or firmware binaries are committed.

Host: macOS arm64, Apple clang 21.0.0, `-ffp-contract=off`.
Initial whole-host build failed on Darwin BSD typedef visibility; rerun with
`X0X_HOST_DEFS=-D_DARWIN_C_SOURCE` passed. Open303 reference checkout was absent;
reference comparison, full-root suite, Elite/Pro/controller target builds and
budgets, hardware, merges and publication are NOT_RUN.

Parent coordinator / sole firmware integrator should reconcile this independent
voice patch with PR99 and the existing probability/TIE/RANDOM stack on one current
candidate, coordinate the separate duration worker, then run combined independent
QA and native Elite/Pro development builds with actual budgets. The designated
tracker writer should record these receipts without overwriting prior history.
This source branch is not an installable NIGHTLY or hardware result.
