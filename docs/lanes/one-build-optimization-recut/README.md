# One-build optimization recut

This recuts the applicable optimization tip patches from
`lane/build-25-all-opt-909v2-20261106@61252430f669ddeb83ed5cac5c3527be2e9e74a4`
onto exactly `a06e20189141ff5fb8766e2c6152be460f4356c3` on `wip/one-build`.
Repeated DSP, control and project-copy work is reduced while the base's sound,
project bytes and seven-effect Chicago policy are retained.

## Scope

[recut-manifest.json](recut-manifest.json) records 33 selected optimization
commits and the strengthened silent-BREAK regression. Three restored-insert
optimizations are inapplicable because that architecture and delay effect are
absent from this base. Their unrelated feature dependencies were not imported.
Historical source-lane evidence was not reused as a fresh test result.

## Verification run here

The root Codex session executed the full host suite and both native DEV builds.
Final host coverage is **334 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN**. The initial full
run was 332 PASS and one new-fixture failure. After the reviewed test-only repair,
`sh tests/run_tests.sh optimization-equivalence` passed with exit 0. The other
332 passes cover unchanged inputs; a second whole-suite green run is not claimed.

During verification, `wip/one-build` advanced to
`8241c057c253422ae6a172560af374c8e12fdbd3`, which independently delivered the same
LZ optimization directly on a06. That commit and its corrected fixture comments
are preserved. Its added standalone `lz-max-match` selector also passed here,
raising the final covered selector count to 334. The delivery parent is 8241;
there is no unrelated ancestry and no force push.
The full result history, commands, log digests and selector matrix are in
[host-suite.json](host-suite.json).

The new characterization checks compare exact PCM, state, controls and persisted
bytes, including real BREAK playback/tails/restart, exhaustive controls, 909 reset,
mono/stereo, signed zero, subnormals and sanitizer runs. Differential scripts use
the actual a06 base. FX state comparison normalizes only the two documented
control-to-coefficient representation changes and independently checked pointers.
No existing test assertions or expected counts were removed or weakened.

Native builds used JieLi Clang 4.0.1 and SDK V1.2.1 commit
`d179b4484759423312073f5fbb232501aa491047`. All 11 consumed app/controller SDK
input pins matched. [inputs.json](inputs.json) and
[production-inputs.json](production-inputs.json) identify the checked closure.
Subsequent repairs changed only tests; all production bytes still match the
measured native candidate. An inherited initializer-braces compiler warning is
present in both base and candidate; the builds complete successfully.

## Measured native deltas

Elite and Pro have the same measured sizes in this configuration.

| Metric, bytes | a06 base | Recut | Delta |
| --- | ---: | ---: | ---: |
| App image | 474840 | 480568 | +5728 |
| App RAM occupied, including alignment | 95780 | 95780 | 0 |
| `.ram_text` | 2712 | 2712 | 0 |
| Pool used | 335464 | 335464 | 0 |
| Package | 651457 | 651457 | 0 |

App RAM headroom is **2524 B** under the 98304 B budget. Pool headroom is
**8600 B**, above the 8192 B floor. App code has **72104 B** free before the
controller block. `.ram_text` is byte-identical to the base, SHA-256
`52705d9683efd8ba671487161c9f1d73a5305bfe5a66d3dd0570b9432f33a00c`.
The matching controller build passed at 61088 B; its RAM use is 26336 of 26368 B.
[native-metrics.json](native-metrics.json) records sections and artifact digests.

The approximately 7256 cycles/sample target is **NOT_MEASURED**. Host equivalence
and native compilation establish no device cycle or peak-load claim.

## Review and delivery boundary

Independent correctness, testing, project-standards, maintainability, performance
and adversarial source review completed with no findings. Three simplification
lenses found no warranted change. See [review.json](review.json) and its
[test-repair addendum](post-review-test-repairs.json).

This is source delivery with BUILD_ONLY evidence, not a DEV CHECKED record,
release, install or device result. Use existing draft PR #698; no duplicate PR,
protected-ref merge, `.github` edit, flash action, binary, SDK, private-header,
secret or raw-capture addition is part of this delivery.
