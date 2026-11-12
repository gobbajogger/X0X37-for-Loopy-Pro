# Read-only SMK pad diagnostics — 2026-10-27

## Purpose and boundary

All sixteen silent pads need evidence from the input path, not another MIDI or
LED monitor. INLOG argument 6 exposes the already-scanned raw/base/peak/state,
velocity, pressure, mapped note, active curves and ADC/scan progress. Existing
arguments 0-5 are unchanged. `tools/ble_autotest.py --pad-inputs` sends only
command 38, argument 6, and fails closed on BUSY, old or malformed replies.

No thresholds, calibration, mappings, scan code, audio/render path, BLE controller
or persistent RAM are changed. RGB pad calibration is not sensor calibration.
The cause of the reported failure remains unknown; these changes do not prove a
mechanical, electrical or calibration fault. No device was accessed.

Base: `85976ecaed11317de73718589693e034061447a8`, tree
`d618c9fb7597ad35550288d1fb2edc0f80b97c38` (firmware `nightly`). The isolated lane is
`lane/mb-pad-input-diag-20261027`; the parent integrator owns integration.

## Host proof

- `python3 tests/smk_pad_inputs_test.py`: PASS. Compiles the actual writer and
  checks all 16 pads, extrema, 528 injected copy collisions, 1,563 interrupts
  during serialization, counter wrap, three-attempt BUSY, scratch/reply bounds,
  noncanonical/truncated/legacy replies and the no-reset CLI branch.
- `ASAN_OPTIONS=detect_leaks=0 CFLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' python3 tests/smk_pad_inputs_test.py`: PASS.
  Leak detection is disabled because executor ptrace prevents LeakSanitizer's
  process enumeration; AddressSanitizer and UndefinedBehaviorSanitizer ran.
- `X0X_TEST_JOBS=1 X0X_TESTS='diag-size smk-inlog inlog-tool smk-pad-inputs smk-padcal-export smk-padcal-wiring' sh tests/run_tests.sh`: six selectors PASS.
- `tests/smk_pkg_test.py` on both freshly built packages with exact Pro/Elite
  identities: PASS (package CRC/identity/region structure only).
- `git diff --check`: PASS.

## Native proof: BUILD_ONLY, not install qualification

Both actual native builds used the existing JieLi Clang 4.0.1 toolchain and the
three SDK V1.2.1 packaging inputs checked against `tools/build.py` SHA-256 pins.
Commands: `python3 tools/build.py --board smk37 --model pro`, then `--model elite`,
with these exact feature values in the environment:

```
X0X_DEV_PADCAL=1
X0X_BASS_PRESET_PREVIEW=1
X0X_BASS_TAGGED_SOUND=1
X0X_BASS_NOTE_SOUND=0
```

| Native DEV result | Pro | Elite |
| --- | ---: | ---: |
| Image bytes | 352296 | 352296 |
| Code headroom | 200376 | 200376 |
| RAM used / limit | 98224 / 98304 | 98224 / 98304 |
| Pool used / limit | 323000 / 344064 | 323000 / 344064 |

Both builds passed the locked-settings, RAM/pool/code, register-access and
soft-double checks. The earlier default-bass-flag builds are not the full-feature
candidate; only the results above apply. Local logs are retained in the sibling
`smk37-pad-diag-evidence` directory with a manifest of source, tools, SDK inputs,
artifacts and log hashes. No binary or raw capture is committed or published.

- Pro app SHA-256: `23a1f5b9c50a69d8f87ec4b1eb6f73959abe8ba43090a1d225f7c38f65a3aec6`.
- Elite app SHA-256: `e0badb5eca4c86baeee3229047e675f288ee1d98a43f346511c6e4f1319efa14`.

Controller inputs/ABI were not changed. No fresh controller qualification or
complete controller-bound DEV record is claimed. Packages remain BUILD_ONLY;
installation, hardware behavior, timing on the user's unit, firmware publication,
merge and production promotion have not been performed or authorized here.

## Independent review

GPT-6.1 Sol, xhigh, two rounds: APPROVE. S1 requested a short coherence window;
it was resolved by copying only 176 bytes into existing call-scoped editor
scratch before serialization. No interrupts are masked. No findings remain.
The reviewer independently passed normal and sanitizer tests. Usage was not exposed.

Protocol details and limits: [EDITOR_PROTOCOL](../../web/EDITOR_PROTOCOL.md#smk-37-pad-sensor-snapshot-inlog-argument-6).
The upstream author's [hardware map](https://github.com/genkigenki/fm1-to-smk37/blob/main/docs/01-hardware.md#6-analog-inputs)
corroborates two pad mux banks and separate LED output; it is not evidence of the
condition of this user's device.
