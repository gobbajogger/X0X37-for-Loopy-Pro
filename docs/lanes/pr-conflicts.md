# PR #212 and #677 conflict reconciliation

## Source

- Repository: `gobbajogger/X0X37-for-Loopy-Pro` only.
- Base: firmware `nightly` at `66073c68dd80d51a7053432c2c161a8b720a1341`.
- Input PR #212: `9aab1f1deb8da75a0ae3dcae59c5841787f50cd1`.
- Input PR #677: `2d555bee09021d8d71b59a5903eb1c93dd74f8e2`.
- Output lane: `chat/pr-conflicts`; commits authored by the connected GitHub account by default.

## Decisions and user-visible effect

PR #677: replay the ordered USB/BLE MIDI overflow handling, TIMER5 ingress/egress protection and 64-entry BLE queue, plus Chicago effect-history retention, live BPM Delay tempo and continuing effect tails. These changes prevent Note Off arriving before Note On, reversed MIDI transport commands, and prematurely silent/cleared effects.

PR #212: retain the newer, existing engine boundary and production Chicago/Miami wiring. Carry its compatible 303-adapter equivalence and registry-rejection tests into the current host suite. Do not overwrite current engine.c, introduce a third engine or restore experimental Wobble source. The old experimental versioned state codec is not ported: project revision 13 remains the only supported format; no migration, engine-selection schema or project bytes change.

No #684 private headers, binary firmware artifacts, third-party SDK/toolchain files or protected flash changes are part of this source PR.

## Local validation required on exact committed source

```sh
sh host/build_host.sh
X0X_TEST_JOBS=1 X0X_TESTS='synth-engine midi-overflow-order chicago-fx-runtime midi-mq-noteoff-overflow midi-mq-rt-overflow midi-out-mq-noteoff-overflow ble-out-noteoff-overflow ble-out-rt-overflow chicago-bpmdly' sh tests/run_tests.sh
python3 tools/local_proof_plan.py --base 66073c68dd80d51a7053432c2c161a8b720a1341 --head HEAD --branch chat/pr-conflicts
python3 tools/dev_validate.py run --commit "$(git rev-parse HEAD)" --model elite --plan docs/lanes/pr-conflicts-dev-plan.json --output build/pr-conflicts-dev-elite
```

Use BUILDING.md's verified AC79NN SDK V1.2.1 pin, JieLi toolchain and licensed DSP references. On macOS, run dev validation in the verified Linux/amd64 worker with `--worker-image x0x37-validation:local` and correct mounted input paths. Verify the resulting `DEV CHECKED` record, controller/package hashes and source tree before integration. The source replay itself **does not constitute a host test, firmware build, device test or release check**. Do not merge or publish on a static patch check alone.
