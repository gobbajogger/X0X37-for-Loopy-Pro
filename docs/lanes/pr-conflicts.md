# PR #212 and #677 conflict reconciliation

## Source

- Repository: `gobbajogger/X0X37-for-Loopy-Pro` only.
- Base: firmware `nightly` at `66073c68dd80d51a7053432c2c161a8b720a1341`.
- Input PR #212: `9aab1f1deb8da75a0ae3dcae59c5841787f50cd1`.
- Input PR #677: `2d555bee09021d8d71b59a5903eb1c93dd74f8e2`.
- Output lane: `chat/pr-conflicts`; commits authored by the connected GitHub account by default.

## Decisions and user-visible effect

PR #677: replay the ordered USB/BLE MIDI overflow handling, TIMER5 ingress/egress protection and 64-entry BLE queue, plus Chicago effect-history retention, live BPM Delay tempo and continuing effect tails. These source changes are intended to prevent Note Off arriving before Note On, reversed MIDI transport commands, and prematurely silent/cleared legacy Chicago effect tails. Their effect on the original Chicago/Miami renderer is not yet validated.

PR #212: the adapter-equivalence and registry-rejection harnesses are preserved as **legacy historical coverage only**. This candidate's `engine.c` still has the older `bass303_render`/`synth303_binding` implementation; it does **not** implement the currently selected original Chicago/Miami runtime. The current original-runtime integration is separately present on `wip/one-build` (verified at `7c32866789a6742c93f6e6c492385596c4a8ec7f`). Do not pull that whole tree into this PR: it includes unrelated features and a committed controller image. The older experimental state codec is deliberately not ported. Project revision 13 remains the only accepted format.

No #684 private headers, binary firmware artifacts, third-party SDK/toolchain files or protected flash changes are part of this source PR.

## Local validation required on exact committed source

```sh
sh host/build_host.sh
X0X_TEST_JOBS=1 X0X_TESTS='synth-engine midi-overflow-order chicago-fx-runtime midi-mq-noteoff-overflow midi-mq-rt-overflow midi-out-mq-noteoff-overflow ble-out-noteoff-overflow ble-out-rt-overflow chicago-bpmdly' sh tests/run_tests.sh
python3 tools/local_proof_plan.py --base 66073c68dd80d51a7053432c2c161a8b720a1341 --head HEAD --branch chat/pr-conflicts
python3 tools/dev_validate.py run --commit "$(git rev-parse HEAD)" --model elite --plan docs/lanes/pr-conflicts-dev-plan.json --output build/pr-conflicts-dev-elite
```

Use BUILDING.md's verified AC79NN SDK V1.2.1 pin, JieLi toolchain and licensed DSP references. On macOS, run dev validation in the verified Linux/amd64 worker with `--worker-image x0x37-validation:local` and correct mounted input paths. Verify the resulting `DEV CHECKED` record, controller/package hashes and source tree before integration. The source replay itself **does not constitute a host test, firmware build, device test or release check**. Do not merge or publish on a static patch check alone.

## Mac qualification failure: recorded, not yet repaired

Owner report for commit `f3da353be71e4639d04c1d842f72568d2af9186d` on 2026-10-29:

- Elite development build: **PASS (reported on the Mac)**, with no artifact bytes/record attached to this repository.
- Host suite: **208 PASS, 101 FAIL (reported on the Mac)**. This is a FAIL, not a partial success.
- Detail file: `codex-lanes/test-pr-conflicts/TEST-RESULT.md` on the owner's Mac. It is **not** present in the GitHub branch, available ChatGPT files or this Linux runner, so the individual failure names and logs were not independently inspected.
- Independent QA review on PR #695 (comment `6503191880`) separately confirmed a source-profile mismatch: `synth-engine` runs original-303 adapter checks and `chicago-fx-runtime` links the old `bass303.c` and `experiments/wobble/wobble.c`. These do not qualify the selected original-runtime Chicago/Miami implementation.
- The existing `pr-conflicts-dev-plan.json` is a **legacy/scoped diagnostic plan only**. Its old-engine checks must not be used as original-runtime acceptance or as a complete project-revision-13 qualification.

**Repair rule:** Classify the 101 failures from actual Mac logs. Fix the first shared root cause, retain negative test coverage, then rerun affected tests. Preserve the existing original-runtime integrator's branch. Port only required portable MIDI queue fixes and applicable Chicago behavior onto that correct source, rather than importing old engines or the other branch's unrelated artifacts. Do not mark this PR ready until host and exact-candidate development checks pass.

### Exact Mac report and failure triage commands

Run from the Mac terminal. These commands read the existing report and logs and do not modify any firmware, protected branches or hardware:

```sh
REPORT=$(find "$HOME" -type f -path '*/codex-lanes/test-pr-conflicts/TEST-RESULT.md' -print -quit 2>/dev/null)
if [ -z "$REPORT" ]; then
    printf '%s\n' 'Mac report not found under HOME: codex-lanes/test-pr-conflicts/TEST-RESULT.md'
else
    printf 'Mac report: %s\n' "$REPORT"
    cat "$REPORT"
    printf '\n--- Failure headings ---\n'
    grep -nEi 'fail|error:|undefined|assert|exception|timeout|not.run' "$REPORT" | head -100 || :
fi
```

For the exact candidate source checkout, use a clean update and only the affected host regressions first. Do not overwrite the Mac's original test report:

```sh
git fetch origin chat/pr-conflicts
git switch chat/pr-conflicts
git pull --ff-only origin chat/pr-conflicts
git rev-parse HEAD
sh host/build_host.sh
X0X_TEST_JOBS=1 X0X_TESTS='midi-overflow-order midi-mq-noteoff-overflow midi-mq-rt-overflow midi-out-mq-noteoff-overflow ble-out-noteoff-overflow ble-out-rt-overflow chicago-bpmdly' sh tests/run_tests.sh
```

To inspect per-test failures after a full suite, inspect `build/host/.results/*.fail` and matching `build/host/<test>.log` files, preserving the original run's artifacts before a rerun. **Do not infer that these commands have run from this document.**
