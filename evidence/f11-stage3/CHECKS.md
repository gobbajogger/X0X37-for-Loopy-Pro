# Stage 3 checks for F11 (BF-007)

Branch: `cursor/f11-bf007-stage3-fb90`
Stage-3 owner: `bc-2bc7a358-dad4-5b70-b3e3-d4a3e7b43fef`
Implement tip under test: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d` (`cursor/f11-bf007-stage2-fb90`)
BF-007 repair commit: `78a0452e` — Fix BF-007 legacy BD PAN motion migration order
`project.c` blob: `240a4b1735166dfb990b52c26686c7ea5ad4317e` (matches implement tip)
Audited / validated source SHA: `af4947846df7233c68a6be72c4d0c634052a2752`
Content tree: `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`
Git tree (pre-repair): `c12a7e747b79aa4619466764ecccc88285d4c10d`
Firmware tree (pre-repair): `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006)
Test run subagent: `bc-27ab5431-773a-5a81-8b8a-c9c13fbc45c9`
Failure investigation subagent: not started (no failing required check)
No device written. No source patch by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.

## Required BF-007 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| legacy BD PAN motion harness | `sh tests/host/run_bugfinder_legacy_bdpan_motion.sh` | 0 | PASS — 171/171 | `evidence/f11-stage3/bf007-legacy-bdpan-motion.log` |
| host unit suite | `X0X_TESTS="legacy-bdpan-motion project-stage project-protocol project-runtime" sh tests/run_tests.sh` | 0 | PASS — all four ok (Pillow 13.0.0 installed for project-runtime) | `evidence/f11-stage3/host-unit.log` |
| project-runtime alone | `X0X_TESTS="project-runtime" sh tests/run_tests.sh` | 0 | PASS | `evidence/f11-stage3/project-runtime.log` |
| migration-order review | read-only + `git rev-parse HEAD:firmware/src/app/project.c` | 0 | PASS — `sound_bd_sends` static PAN post-OBJ_SOUND; `sound_bd_sends_lanes` after stream/arrange; blob match | `evidence/f11-stage3/bdpan-migration-review.txt` |

Machine summary: `evidence/f11-stage3/test-run-summary.json`. Full run: Project store `internal/f11-stage3-test-run.md`.
Optional env install: `evidence/f11-stage3/agent_env_install.log` (Pillow was missing initially; installed then suite re-run).

## Migration-order detail

- `sound_bd_sends` remaps static `pr->sound.v[t][0]` PAN only, called immediately after OBJ_SOUND load when `snd_rev` is low.
- `sound_bd_sends_lanes` remaps arrangement lane indices only after stream/arrange load, before `snd_rev` bump.
- Blob `240a4b1735166dfb990b52c26686c7ea5ad4317e` matches implement tip.

## Failures

None. Every required check met its expected exit. No cause/disposition rows.

## Overall

**ALL_PASS** — required BF-007 host regressions. Deferred items unchanged: `evidence/f11-stage2/DEFERRED.md`. Open PR #28 `fastmath.h` not required.
