# Optional engine scratch reuse: 1 KiB main-RAM saving

## Change and ownership

Base: `2695701fe9ae63dbb6f1f76d4dc11ebbf39da834`.
`firmware/src/app/engine.c` is byte-identical at the current named DEV source
`19ab0b239e97e471487b94f24301794331840de6` before this patch.
This optional lane does not modify or repin the integration owner's delivery
candidate. No radio producer, hardware operation or release gate was run.

The standalone 909 `mono[256]` scratch becomes the current `out_l` slice:
- The 909 kit send and meter consume it before either bass renderer runs.
- Bass rendering fully overwrites that slice before subsequent mixing uses it.
- Core 2 accesses only `t0/tr/t1/t2/kbuf`, never `out_l`.
- `engine_render` owns private output arrays and passes `out_l + done`, so
  previously completed slices are untouched. The existing non-reentrant engine
  ownership contract is unchanged.

## Target measurements

Exact named Elite DEV baseline app rebuilt byte-for-byte:
`2177ff0a2a6afe96564665ec3588fdf2634fc4ae8761354beb39735e69eb0826`.
Verified target compiler SHA256:
`c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`.
The two scoped builds suppressed packaging and produced no installable candidate.

| Budget | Baseline | Patched | Limit |
|---|---:|---:|---:|
| Main RAM | 98100 | 97076 | 98304 |
| Application bytes | 521348 | 521364 | 524000 |
| Pool | 335484 | 335484 | 344064 |
| BLE mailbox | 2936 | 2936 | 4096 |

Main-RAM margin grows from 204 to 1228 bytes, costing 16 flash bytes.
Controller, security, buffers, heaps, stacks, pool guards and sample capacity
are unchanged. Static margin is not a runtime stack/heap high-water measurement.

## Differential tests and limits

`tests/host/engine_scratch_alias_test.sh` compares the baseline engine and current
engine against identical DSP and assets. It checks 4096 full-engine blocks per
mode, sizes 1..256, inactive/active drums, repeated triggers, sequencer subdivision,
solo and voice mute changes. Serial, inline second-core and pthread second-core
outputs match exactly, including PCM, float scratch, mix glides, meters and scope.
Combined snapshot hash for each of all six outputs:
`d054c114f3bec3e12321660c65e6df051fd36ee2c4c8d5c4ba9d2984318fbe05`.

The unrestricted baseline sanitizer run exposed an existing signed-left-shift
warning at drum808.c:184. LeakSanitizer cannot run under this environment's
tracing. The regression explicitly excludes shift sanitization and leak detection;
remaining address/undefined checks halt on error. Do not report full clean UBSan.

Independent reviewer accepted lifetime, core ownership, bounds and target metrics.
Its separate 600-block threaded pre-master float/PCM/glide/meter differential also
matched, SHA256 `e83ac7c95606d74b4dd3b1528849d7b9e11c2bec6ff2902f91eea4d1762c50b2`.
Review retained identical pre-existing shift warnings, no additional findings.

Detailed local evidence: `controller-ram-optimization/REPORT.md`, `budgets.json`,
`target-baseline-build.log`, `target-build.log`, `equivalence-state.log` and
`regression-script.log`; independent report `optimization-equivalence-review/REVIEW.md`.
No raw captures, target binaries, SDK or toolchain files are included in this commit.

The study bridge overlay now includes `firmware/src/app/engine.c`; otherwise its
816e export would silently retain the old scratch allocation. Verified a fresh
816e export through prepare, admission and bridge steps and compared the resulting
engine source byte-for-byte with the reviewed candidate. The packaged DEV builder
remains under integration-owner control; this source-selection fix is not delivery
approval or a new package.
