# Original Chicago and Miami on the published firmware release

This candidate starts from `0e77d923dfee44a9b64a98c121fac55200b91f20`
(tree `2b4c32cf531dd3402cc33895e9dbc66ad1aebace`). This is the source of
`nightly-0e77d923-dev`, the latest installable firmware entry in the NIGHTLY
catalog inspected on 2026-10-29. The catalog repository is
`gobbajogger/x0x37-for-Loopy-Pro_nightly`, pinned at
`821aa12e6625dd6e7b3c5202d7d0838a420d0bdc`. The installer embeds the same row.
The source commit, every Git blob and executable mode were verified locally.

The owner rejected the previous `ca8299b6` shared UI base. This candidate uses
the published release's drawing, fonts, four-column controls, piano roll,
HOME/MIX screens and refresh cadence. The new UI code supplies original synth
pages and exact value formatting to that renderer. Existing `ui` and `pg`
objects use `X0X_POOL` only in the original synth profile: the first real
Elite link otherwise exceeded ordinary static RAM by 152 bytes. This changes
placement, not the object count, rendering or input behavior.

## Control mapping

Visible names follow the plugin panels rather than internal DSP field names.
Original factory values remain binary32 values; displaying a quantized encoder
position does not overwrite those values. The compact profile contains 55
Chicago and 30 Miami factory programs.

| Engine | Page | Panel controls |
|---|---|---|
| Chicago | OSC | WAVE, GLIDE |
| Chicago | FILTER 1 | CUTOFF, PEAK, ENV |
| Chicago | FILTER 2 | BITE, GNAW |
| Chicago | ENV | ATTACK, DECAY, SUSTAIN, RELEASE |
| Chicago | AMP | ENV/GATE, LEVEL |
| Chicago | ARP | ON, RANGE, MODE, SPEED |
| Chicago | EFFECT | TYPE, EDIT 1, EDIT 2 |
| Miami | CARRIER | WAVE, LEVEL, SUB, PITCH |
| Miami | XMOD | WAVE, LEVEL, TUNE, DEPTH |
| Miami | FILTER | CUTOFF, CRUSH, WOBBLE |
| Miami | WOBBLE | SHAPE, RATE |
| Miami | OUTPUT | GLIDE, STEREO, OUTPUT |

Pitch Bend and Damper are runtime performance inputs, excluded from pages and
stored motion. Pitch Bend follows the physical wheel and MIDI; its centre is
neutral. The physical modulation wheel controls Chicago CUTOFF or Miami
WOBBLE. Miami's CARRIER PITCH is the separate oscillator tuning control and
remains visible. XMOD TUNE uses its original unitless scale.

Chicago's compact effects are OFF, CHO/FLG, PHASER, RING MOD, COMPRESSOR,
DECIMATOR and EQ. They retain native type IDs 0, 4, 5, 10, 18, 22 and 24.
The production build excludes the retired Bass303/Wobble DSP objects and the
full simultaneous effect-history owner. Reference-profile code is not a
compatibility migration path for old saved engine data.

## Exact idle optimization

The output owners prove that the entire input span is zero and that the
relevant signal history and controls are at an exact fixed point. Only then
can conversion, settled Drive, wet-mix and effect signal work be skipped.
This is not an amplitude threshold or a global inactive-voice sleep switch.

The original scheduler, voice/global work and master processing remain.
Ring/Phaser phase, Chorus modulation and delay head, decimator phase and pending
step semantics advance as in the original implementation. Events that require
processing, transitions, countdowns and evolving histories take the existing
processing path. Bypass retains its original unconsumed event cursor even on
the fast path. A refusal cannot partially advance state before falling back.

EQ requires a constant-output proof rather than a zero-output assumption:
its quantized recurrence can settle at a nonzero residual sample after a
played note. The optimized path preserves that sample, wet blend, history and
master output exactly. No history is cleared to manufacture silence.

The focused differential fixture compares PCM, semantic state and used scratch
buffers through cold idle, played release, settled idle and the next note. It
also covers signed zero, both FZ modes, small and boundary-sized spans,
transactional refusal, pending/reset events, an actual Chicago arpeggiator
wake and Miami program replacement. Its normal and ASan/UBSan runs must agree.
Structural counters report skipped original work separately from timing;
they are not estimates of device CPU percentages.

## Application boundaries and reproduction

Physical keyboard events retain independent source tokens and a project epoch.
Accepted but undrained input blocks project/undo transfer. A project boundary
revokes the previous epoch, preventing a held outer key from becoming a note
in a replacement project. Pending UI Stop is consumed before subsequent CQ
note commands. Stale synth-page/list identities cannot edit the newly selected
engine through old parameter IDs.

With the separately verified external inputs available, the focused commands
are:

```sh
python3 tools/check_original_inputs.py --scope active --json
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 sh tests/host/original_idle_test.sh /tmp/fresh-original-idle
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 sh tests/host/original_release_integration_test.sh /tmp/fresh-release-integration
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 sh tests/host/original_release_ui_test.sh /tmp/fresh-release-ui
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 python3 tests/host/engine_cq_isr_preemption_test.py --sanitize
```

Raw external plugin tables, SDK files and toolchains are not source artifacts.
The plugin table/factory input hashes and active-profile scope are in
`experiments/original_synth/EXTERNAL-INPUTS.json`. SDK and toolchain pins belong
to the separate native build receipts.

Real Elite/Pro/controller build receipts must bind the exact candidate commit,
tree, toolchain, external inputs, linked object set and package bytes. Host
timings cannot establish the device's reported 79%/120% idle load or its worst
interrupt/DMA/USB deadline. That acceptance requires measurements of the same
firmware package on the matching hardware; BUILD_ONLY does not claim it.
