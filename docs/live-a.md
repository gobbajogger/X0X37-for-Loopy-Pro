# LIVE-A source handoff

This draft contains L1, L11 and L12 measurement tooling on `build/0.0.23` at `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`. The owner requested source and regression tests here, with all execution assigned to the Orchestrator on the box. No builds, tests, simulator runs, loudness measurements or device operations were run in Codex. This is not firmware qualification.

## Behavior and review limits

L1 ignores recognized legacy six-byte OTA/UBOOT entry commands while transport is playing, start/continue is pending, the engine reports held/queued notes, or accepted USB Start/Continue/nonzero Note On is still waiting in the ingress ring. The bounded pending-ring scan does not consume or reorder MIDI. Each refused complete command increments `usb.ota_refused`; stopping does not revive it. Fresh stopped requests still use the existing path. The change is confined to `usb.c`, with application-only engine dependencies; SysEx matching/terminators are unchanged to keep QB's separate work independent.

**BUSY limitation:** these legacy commands have no existing BUSY reply envelope. The existing editor install BUSY handler does not read the counter or transport state. This draft does not invent a wire response, modify editor code, or claim ED_INSTALL protection. The refusal counter is an internal diagnostic. A BUSY reply remains outside this usb-only implementation.

L11 gives the already-open BREAK gate its existing fade target before each render stretch. Quiet alone only stops decoding after the gate has closed. Reading the mute directly avoids the command queue's 128-of-1024 drain delay. Unmute still waits for the next actual step and its probability decision. Held-slice audition retains the existing override, and FX tails are preserved. The one-block regression concerns dry sequenced BREAK output without a held slice.

**ISR limitation:** this implementation reads the mute at each existing render stretch and, while muted, applies the quiet setter plus two gate-state stores. It adds no per-sample loop or production state, but these bounded control updates exceed a literal “flag read only” requirement. Keep this explicit for Orchestrator review; no measured deadline or CPU cost is claimed. There are no `main_fm1.c`, DSP-header, UI or level changes.

The required 32-bit USB refusal counter is expected to cost four application RAM bytes; this diagnostic is the written reason for that expected reduction. Actual Elite/Pro free RAM must come from native map files. L11 adds no production data declaration. The brief's 2352-byte baseline is reported history, not a new measurement.

## Box validation

Run on the exact draft head. The new tests are registered in `tests/run_tests.sh`:

```sh
sh tests/run_tests.sh usb-live-update engine-brk-mute live-a-mix-report
sh tests/run_tests.sh
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
X0X_DEV_PADCAL=0 MODEL=pro ./build.sh --board smk37
```

Use the repository's pinned SDK/toolchain and native build instructions. Record full-suite fail/skip counts, both native build results, exact source/artifact identities and static free RAM separately. Do not infer native or hardware PASS from source review. No FM-1 target, device install, flash, release or publication is requested.

The USB regression uses the real parser and USB state with explicit engine-query/transport boundary doubles. It covers independent live conditions, undrained USB intent through the real ring writer, index wrap, completion-time transitions, refusal counts, replay, unrelated SysEx and loader independence; it does not run engine note ownership or a USB endpoint.

The BREAK regression extracts production engine/CQ/control bodies and uses the real BREAK renderer on a deterministic source. It includes a slow-step deadline, sample-step bound and abrupt-cut positive control, queue saturation, render splits, song events, unmute/probability behavior and held/stopped audition. It stubs unrelated synths, sequencer timing and FX/master; actual CPU2 dispatch, full engine integration and native timing remain box checks.

## L12: measure without changing levels

`tools/mix_report.py` is unchanged. `tools/live_a_mix_report.py` reuses its scenario/analyzer functions and creates a host-only adapter because the existing simulator cannot both select and verify admitted bass banks through commands. The compiler shim substitutes the host translation unit in the unchanged `host/build_host.sh` command; it changes no tracked host/firmware source. Other compiler calls, including the built-in BREAK asset generator, pass through to the configured real compiler unchanged. Review this adapter along with the resulting provenance before using its measurements.

Use the same candidate helper for a clean baseline checkout and a clean candidate checkout. Run these steps in each source root, using separate output paths:

```sh
python3 /path/to/candidate/tools/live_a_mix_report.py --source-root "$PWD" --prepare-host --outdir build/live-a-host
CC="$PWD/build/live-a-host/cc-live-a" X0X_HOST_OUT=build/live-a-host/x0x_host sh host/build_host.sh
python3 /path/to/candidate/tools/live_a_mix_report.py --source-root "$PWD" --host build/live-a-host/x0x_host --outdir build/live-a-measured --label before
```

Use `--label after` in the candidate root. Keep source roots fixed between compiling and measuring. Use identical generated assets and either both external THINK/FUNKY sample files or neither. The existing analyzer requires NumPy and SciPy.

Compare the resulting reports:

```sh
python3 /path/to/candidate/tools/live_a_mix_report.py --before /path/to/base/build/live-a-measured/report.json --after /path/to/candidate/build/live-a-measured/report.json --outdir /path/to/comparison
```

Attach `table.md`, `table.csv`, JSON reports/provenance and the source/build/sample identities to the draft. The helper produces factory-sound programmed mix rows, each part solo, individual 909 voices, and Chicago/Miami solo and mix rows in both bass slots with matched musical lines. Runtime bank/preset/admission checks guard the labels. The measurements are ungated BS.1770 K-weighted left-channel LU and left-channel sample peak, not gated stereo integrated LUFS or true peak.

No levels, bank trims, presets, Miami ADSR or factory sound defaults are changed. There is no listening approval or loudness result yet.

## Current evidence

| Check | State |
|---|---|
| Focused regressions and full host suite | NOT_RUN |
| Elite / Pro X0X_DEV_PADCAL=0 | NOT_RUN |
| Elite / Pro static free RAM | NOT_MEASURED |
| Before/after LU and peak | NOT_MEASURED |
| Hardware | NOT_TESTED |

Independent static review is recorded in the draft PR. It does not turn any unrun check into PASS. The Orchestrator owns the next execution stage; retain the draft until its results and the review limits above are resolved.
