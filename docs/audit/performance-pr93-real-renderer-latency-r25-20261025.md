# PR #93 R25: measured callback latency, not a claimed hardware speedup

**User experience.** Musicians expect BREAK slices to stay steady while reversing, crossfading between pads, releasing notes and stopping. R23 optimizes a checkpoint-aligned reverse seek. R24 proved the optimized branch is reached in real host rendering, but did not measure whole-renderer timing. The potential benefit is reduced callback CPU cost and ultimately fewer audio dropouts; neither device-level outcome has been demonstrated.

**Design and exact scope.** Compare immutable R22 pre-change `breaks.c` Git blob `9e1e607e2ddc3028383bfb6ebc85fbc7ac6864c5` to R23 optimized blob `01e0e759d2b8be3b028e0e25d4f93348a36aeb46`. Both are tested using the pinned production dependencies and existing R22 48-case two-voice live MIDI NoteOff/STOP workload, with identical host-only timers around each actual `breaks_render` call. No firmware source, compiled closure or input file is mutated. Baseline is R22 within PR #93, **not current NIGHTLY**. Current PR #93 HEAD at intake was `a7f56778cf24a1c933a8f507eaf0728a7f524614`, full tree `d99ceb058738dbaa76abbe18d08c7fe26d4c3557`, firmware tree `75723d43185b1b7aeeaf4c50615e4381ce623a6c`. The separate source-checkpoint workflow contract at `e2482a5039527195e346925c751ac3e3a333ada9` is a reviewed trial, not a merged policy.

**Measured outcome.** Eleven paired interleaved native GCC and Clang optimized before/after runs were made with process CPU-clock timing, plus eleven paired runs with wall-clock timing. The actual callback workload is 48 cases x 80 blocks x 64 PCM frames per executable. Each pair produced identical PCM and control/counter traces, positive audible frames, active overlapping voices in reverse crossfades, and STOP silence. Missing inputs, mutated source, invalid arguments and evidence overwrites were rejected.

| Clock | Compiler | Baseline median reverse CPU/render time | Optimized median | Paired median apparent speedup |
|---|---|---:|---:|---:|
| Process CPU | GCC | 18.199 ms | 18.846 ms | **-1.100%** |
| Process CPU | Clang | 24.984 ms | 26.233 ms | **-3.826%** |
| Wall / monotonic | GCC | 16.483 ms | 17.114 ms | **-5.892%** |
| Wall / monotonic | Clang | 25.739 ms | 25.510 ms | **-4.735%** |

Times aggregate 24 reverse-playing scenarios each containing 80 real `breaks_render` calls, so they are **not per-callback values**. Paired medians and the separately computed medians of totals are not algebraically identical. Timed samples include symmetric host IMA counter instrumentation and clock reads, unpinned shared-host CPU-frequency/scheduling noise, and artificial ADPCM content. Even the direction and magnitude of apparent performance differ by run and timing basis: **no reproducible whole-renderer speedup is established**, nor do these data alone prove a production timing regression. R23's earlier 6-10% checkpoint-helper microbenchmark improvements remain narrowly valid to that helper, not to the complete callback or hardware.

**Safety and meaning.** All 48 PCM/control case rows match between before/after in all 44 primary program runs; original inherited `fastmath.h:57` sanitizer undefined behavior is separately owned by PR #28, so sanitizer on original source is NOT_RUN/PENDING here. The above checks are producer-owned native host evidence only, not Independent QA or matching-model device validation. Elite/Pro/controller target BUILD_ONLY for R23 compiled inputs remains separately pending with Hourly Build. Changes are test-only and must not trigger identical firmware recompilation without invalidation. The first NIGHTLY/production/installer publication and hardware gates remain separate. No web/browser/device assertions are drawn from host-native timing.

**Decision.** Do not change firmware again based solely on this noisy host timing evidence. Preserve the R23 optimization for Independent QA and target-build/memory-budget assessment, or have the owner reconsider it on new reproducible evidence. No speculative optimization churn while installable-critical gates are pending.

**Reproduce (Linux host GCC+Clang):**

```sh
cd tests/host
python3 run_breaks_r25_renderer_latency.py --before-root ../../before --after-root ../../after --output-dir ../../fresh-r25-cpu --repetitions 11 --clock process_cpu
python3 run_breaks_r25_renderer_latency.py --before-root ../../before --after-root ../../after --output-dir ../../fresh-r25-wall --repetitions 11 --clock monotonic
```

The runner independently checks Git blob identities for production code, dependencies and stress test. It refuses existing output destinations and emits timed logs and exact-result JSON. CPU clocks/scheduling influence numbers on a new machine; reruns must not be misrepresented as identical timing proofs.

**Next owners.** Existing Independent QA: review R23 firmware blob, real-renderer test coverage, original sanitizer ownership and current exact PR revision. Existing Hourly Build: compile exact R23 changed firmware tree if not already genuinely covered, pin Elite/Pro/controller closure and budgets; independent delivery not hostage to hosting. Only Safe Merge controls source integration. No work was offloaded without a supported execution endpoint.
