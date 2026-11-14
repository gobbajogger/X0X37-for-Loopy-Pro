# PR #895: matched host follow-up and Runner handoff

**Source-proven disposition: keep #895 draft.** The one bounded matched host campaign completed, but **0 of 20 same-function A/A controls qualified**. The result remains **INCONCLUSIVE**. No established performance regression was accepted; neither a speedup nor a never-slower result is claimed. Device CPU remains unmeasured by this lane.

This follow-up adds a timing driver, raw evidence, and a handoff to the same PR. Production DSP is unchanged from `eef741997e743189f17df7e46bf881e472ad254c`; its actual comparison parent is `1322293e89c1582eecb3d9b7950e14db01722065`. The native binaries and existing PCM/state evidence remain pinned to that source candidate even after the PR head advances for these evidence-only files. No native rebuild or existing PCM/state rerun was performed.

## Evidence vocabulary and scope

- **Source-proven** means directly readable source, retained output, or an explicitly described calculation from those records. It does not imply coverage beyond the recorded workload or a hardware measurement.
- **Estimate** would mean an unmeasured quantitative projection. This follow-up supplies no target-cycle or speedup estimates.
- **Hypothesis** means an unverified explanation. Host frequency/virtualization effects may contribute to the noise, but the cause has not been established. It would be incorrect to attribute all variation to descheduling.
- The Runner instructions below are a **proposed measurement procedure**, not completed measurements. This evidence format is unrelated to J6/plugin preset schemas or UI frames. It makes no J6 identity or compatibility claim.

## What the bounded host run controls

**Source-proven:** [protocol.json](host-matched/protocol.json) was written before compilation/execution and its hash is bound by [summary.json](host-matched/summary.json). [commands.json](host-matched/commands.json) records all compiler/link commands. The [C driver](../../tests/host/juno6_matched_bench.c) and [Python runner](../../tests/host/juno6_matched_bench.py) are hash-pinned by the protocol.

The runner checks the actual-parent fixture and shared-header hashes, plus the unchanged candidate source hash. It uses GCC 14.2.0, `-Os` and `-O2`, `-fno-fast-math`, and `-ffp-contract=off`, without LTO. Both translation units use identical settings apart from symbol renaming needed to link both implementations. Both object link orders are retained, with fixed execution order Os/AB, O2/BA, Os/BA, O2/AB. Every recorded leg remained on CPU 0.

Each leg starts from the same complete synth/chorus state, copied outside timing. It performs sixteen 256-frame blocks of render, mono-to-right copy, and chorus at 44.1 kHz. The five cases are idle, one/six held voices, and one/six releasing voices. The driver warms both variants equally, alternates ABBA/BAAB and comparison/control-first order, rotates the case order, and interleaves an A/A control using the **identical parent function pointers for both labels**. The A/A control is not a separately renamed clone.

There are 15 trials per case/configuration, 16 comparison quartets plus 16 control quartets per trial. All **9,600 quartet records / 38,400 timed legs / 157,286,400 timed frames** are retained. No outliers were deleted and there was no repeat-until-pass. Thread CPU nanoseconds, monotonic wall nanoseconds, CPU IDs, and voluntary/involuntary context switches are recorded. State/final-output checksums are outside timing; these 32-bit work guards do not replace the previously completed byte comparisons.

**Source-proven analysis rule:** aggregate the A and B CPU times within each trial, then report the median of the 15 B/A ratios. The interval is the third through thirteenth ordered ratio: a sign-test median interval with nominal 99.2615% coverage under independent, identically distributed observations. Temporal dependence or changing conditions weaken that interpretation; it is not a universal confidence statement. The predeclared A/A precision gate requires its interval to include 1 and fit entirely within [0.99, 1.01]. That 1% band is a measurement noise check, **not permission for a 1% regression**.

## Recorded ratios

**Source-proven:** values below are medians with the nominal median interval in brackets; B/A above 1 means greater measured B CPU time. For A/A both labels execute the same code. For A/B, B is the candidate. Every row failed its A/A precision gate, so every A/B classification remains INCONCLUSIVE.

| Compiler / link order | Case | A/A control | Candidate / parent | Result |
|---|---|---|---|---|
| Os / AB | idle | 1.0509 [0.7561, 1.3417] | 1.0698 [0.9607, 1.2295] | INCONCLUSIVE |
| Os / AB | held-1 | 0.9949 [0.8548, 1.4998] | 1.0559 [0.7328, 1.3220] | INCONCLUSIVE |
| Os / AB | held-6 | 1.0324 [0.9102, 1.2945] | 1.0239 [0.8033, 1.0989] | INCONCLUSIVE |
| Os / AB | release-1 | 0.9911 [0.8617, 1.4012] | 1.1198 [0.8306, 1.4635] | INCONCLUSIVE |
| Os / AB | release-6 | 0.9733 [0.8835, 1.2728] | 1.0663 [0.9248, 1.1885] | INCONCLUSIVE |
| O2 / BA | idle | 0.9966 [0.8779, 1.1431] | 0.8992 [0.7250, 1.0763] | INCONCLUSIVE |
| O2 / BA | held-1 | 1.0044 [0.8268, 1.3709] | 0.9380 [0.5673, 1.5932] | INCONCLUSIVE |
| O2 / BA | held-6 | 0.9446 [0.9018, 1.0545] | 1.0685 [0.9274, 1.1588] | INCONCLUSIVE |
| O2 / BA | release-1 | 0.9545 [0.8628, 1.1475] | 0.9801 [0.7177, 1.2550] | INCONCLUSIVE |
| O2 / BA | release-6 | 0.9841 [0.8329, 1.2048] | 0.9726 [0.7461, 1.0916] | INCONCLUSIVE |
| Os / BA | idle | 1.0391 [0.8337, 1.1785] | 0.8931 [0.7304, 0.9369] | INCONCLUSIVE |
| Os / BA | held-1 | 1.1679 [0.8955, 1.3958] | 0.8728 [0.7531, 1.3459] | INCONCLUSIVE |
| Os / BA | held-6 | 0.9911 [0.8807, 1.1316] | 0.9979 [0.9140, 1.1127] | INCONCLUSIVE |
| Os / BA | release-1 | 0.9783 [0.8899, 1.0927] | 0.9171 [0.8176, 1.0141] | INCONCLUSIVE |
| Os / BA | release-6 | 0.9983 [0.9174, 1.0609] | 1.0328 [0.9702, 1.0938] | INCONCLUSIVE |
| O2 / AB | idle | 1.1091 [0.8990, 1.3345] | 1.0939 [1.0034, 1.4015] | INCONCLUSIVE |
| O2 / AB | held-1 | 1.0434 [0.8980, 1.2233] | 1.0661 [0.9480, 1.1722] | INCONCLUSIVE |
| O2 / AB | held-6 | 0.9627 [0.7846, 1.0548] | 1.0461 [0.9741, 1.1305] | INCONCLUSIVE |
| O2 / AB | release-1 | 1.0421 [0.7847, 1.2019] | 0.9360 [0.8948, 1.1450] | INCONCLUSIVE |
| O2 / AB | release-6 | 1.0262 [0.9380, 1.1563] | 1.0130 [0.7450, 1.1270] | INCONCLUSIVE |

The four raw files are [Os/AB](host-matched/raw-Os-AB.jsonl), [O2/BA](host-matched/raw-O2-BA.jsonl), [Os/BA](host-matched/raw-Os-BA.jsonl), and [O2/AB](host-matched/raw-O2-AB.jsonl). [validation.json](validation.json) records an exact recomputation of every summary from the existing raw files; it did not render audio or rebuild firmware. It also checks record counts, hashes, CPU IDs, and per-case checksum consistency. [run.log](host-matched/run.log) records completion of all four configurations.

**Source-proven limitation:** even the apparent Os/BA idle improvement and O2/AB idle slowdown have unqualified controls and change direction across layouts. Neither can be promoted into an established algorithmic result. Across the four runs, median wall/thread-CPU ratios are approximately 1.0015; the thread-CPU variation therefore cannot simply be discarded as wall-clock preemption. All raw samples, including slower candidate values, remain visible.

## Why further measurement stops here

**Source-proven:** [host-environment.json](host-environment.json) records the reported virtualized EPYC host, available tools, and read-only capability inspection. The tested hardware-cycle interface returned `perf_event_open = -1`, errno 2 (`No such file or directory`). `perf`, `valgrind`, and host `clang` were not found. Frequency-governor/current-frequency and cgroup CPU-accounting/pressure files at the inspected paths were absent. No fixed-frequency or isolated execution condition was demonstrated. These observations describe unavailable controls; they do not identify the cause of variance or establish that every alternative measurement facility is unavailable.

The old [wall/CPU timing](reused/host-timing.jsonl), [CPU follow-up](reused/host-cpu-timing.jsonl), and [identical-source control](reused/identical-source-timing-control.jsonl) are retained without alteration. The new matched run improves the within-run controls, but its same-function controls still fail. Repeating the same uncontrolled campaign until a favorable result appears would not qualify the host. A controlled host with a qualified A/A measurement is the concrete remaining host input. The evidence supports stopping this environment's attempts; it does not prove never-slower behavior or an unavoidable performance regression.

## Exact native and PCM/state evidence reused

**Source-proven:** application and package hashes were revalidated against the existing [native build receipt](reused/native-build-receipt.json). [runner-manifest.json](runner-manifest.json) adds current hashes/sizes for the existing ELF, disassembly, and linker script, and pins SDK/compiler archives, compiler bytes, configuration, source commit/tree, actual parent, and source files. Binary contents are not included in this commit.

| Model | Application bytes | Flash free | Margin above 8,192 | Pool free | Pool margin above 8,192 |
|---|---:|---:|---:|---:|---:|
| Elite | 544,264 | 8,408 | 216 | 8,340 | 148 |
| Pro | 544,256 | 8,416 | 224 | 8,340 | 148 |

The [Elite](reused/elite-build.log) and [Pro](reused/pro-build.log) logs and existing package checks are reused. These margins qualify only this exact source/configuration; they do not reserve space for queued features.

[PCM/state](reused/exact.jsonl), [ASan/UBSan](reused/sanitized-final.jsonl), and [parent characterization](reused/parent-characterization.jsonl) records remain unchanged. Each Os/O2 comparison reports 19,074 byte comparisons over 290,875 frames, including complete 352-byte synth and 536-byte chorus state. The retained [negative control](reused/negative-control.stderr) rejects skipping the dying-sample update. The [original production review](reused/independent-review.json) is reused and explicitly remains Not ready for CPU qualification. None of those checks was rerun for this evidence-only follow-up.

## Existing Runner owner: next action

Use [RUNNER-HANDOFF.md](RUNNER-HANDOFF.md) and the machine-readable [runner-manifest.json](runner-manifest.json). Resolve the **existing exact-parent native artifacts** and access to the listed candidate bytes, then attach matched counter results or the precise instrumentation/access limitation to the same PR. No new Runner, emu360 lane, native candidate build, device write, merge, or publication is requested.

**Source-proven delivery boundary:** this producer has candidate files and hashes, but has not verified Runner access to those files, the exact parent's native binary/ELF, or Runner consumption of this handoff. The live `coordination/merge-queue` ref probe returned HTTP 404, so this record does not invent a role/request identity or claim a new queue dispatch. A device result also does not automatically close the separate unresolved host gate.

## Focused follow-up review

**Source-proven:** the [completed review](review/review.json) covers the two new timing files and their retained raw evidence. Six read-only review lenses found no actionable defects; [independent recomputation](review/raw-verification.json) reproduces the source/protocol/raw/executable hashes and every timing summary. The review permits an evidence-only draft update and explicitly leaves firmware integration **Not ready**. No DSP execution, existing PCM test, or native build was repeated by the reviewers. Handoff prose and local evidence links were checked by the owning executor.

## Reproduction and acceptance

The already completed host invocation was:

```sh
python tests/host/juno6_matched_bench.py --cpu 0 --out evidence/juno6-release-eef741997e74/host-matched
```

It refuses to overwrite the retained run. Do not execute it again in this environment to seek a pass. For a justified controlled-host comparison, retain the same source/protocol and choose a new, explicit output directory, recording the host controls. No PCM/native rebuild is implied.

Current acceptance: PCM/state **PASS (reused)**; Elite/Pro flash/pool **PASS (reused)**; host CPU **INCONCLUSIVE**; target CPU **NOT RUN**. Keep the draft. Reject any established matched regression; do not treat insufficient resolution as acceptance.
