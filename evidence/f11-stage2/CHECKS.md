# Stage 2 checks for F11 (BF-007)

Branch: `cursor/f11-bf007-stage2-fb90`
Stage-2 owner: `bc-1ef92313-0365-52c7-bb5d-751d2aef133c`
Base audit tip: `f625b57d` (`cursor/f11-bf007-stage1-fb90`, includes Codex P2 witness-runner path fix)
Audited / validated source SHA: `af4947846df7233c68a6be72c4d0c634052a2752`
Content tree: `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`
Git tree (pre-repair): `c12a7e747b79aa4619466764ecccc88285d4c10d`
Firmware tree (pre-repair): `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`
Audited firmware content tree (pre-BF-005 package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair: **BF-007 only** — split BD PAN static vs lane migration in `firmware/src/app/project.c` + host regression `tests/host/bugfinder_legacy_bdpan_motion_test.c` wired as `legacy-bdpan-motion` in `tests/run_tests.sh` + definition `docs/audit/bugfinder-legacy-bdpan-motion-20261023.md`.
`project.c` blob before: `1000a0d88e04553295b82d724a63670434b67748`. After: `240a4b1735166dfb990b52c26686c7ea5ad4317e`.
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.
BF-001–BF-006 and FP-001 not reopened. Open PR #28 `fastmath.h` edits not folded in.
Development configuration for later firmware rebuilds: `X0X_DEV_PADCAL=1`.

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| `bugfinder_legacy_bdpan_motion_test` on unfixed tip (`1000a0d8…`) | **FAIL** exit 1 — 171 checks / 3 FAIL (legacy 909/808 BD PAN motion indices) | `evidence/f11-stage2/bf007-red.log` |
| Same harness after `sound_bd_sends_lanes` post-arrange | **PASS** exit 0 — 171/171 | `evidence/f11-stage2/bf007-green.log` |
| `X0X_TESTS="legacy-bdpan-motion project-stage project-protocol" sh tests/run_tests.sh` | **PASS** — all three ok | `evidence/f11-stage2/host-unit.log` |
| `X0X_TESTS="project-runtime" sh tests/run_tests.sh` | **PASS** — project-runtime ok | `evidence/f11-stage2/project-runtime.log` |

## Repair summary

- Keep `sound_bd_sends()` for static BD PAN values only (post-OBJ_SOUND).
- Add `sound_bd_sends_lanes()` and call it only after stream/arrange load, before `snd_rev` bump.
- Promote lane harness into `tests/host/` and wire `legacy-bdpan-motion` (`-Itests/host` for `project_stage_test.c`).
- Stage-1 tip also fixed `evidence/f11-stage1/run_bugfinder_legacy_bdpan_motion.sh` to compile the evidence source + `-Itests/host` (Codex P2 on PR #257).
- Copy definition to `docs/audit/bugfinder-legacy-bdpan-motion-20261023.md`.

## Deferred

See `evidence/f11-stage2/DEFERRED.md`.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
