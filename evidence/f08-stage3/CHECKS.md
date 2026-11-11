# Stage 3 checks for F08 (BF-004)

Branch: `cursor/f08-bf004-stage3-fb90`
Stage-3 owner: `bc-85452fe2-cacc-5d1e-a23d-189f00c0f7c9`
Implement tip under test: `8a74ff52cba842c3f2d8365600286f94e6bccdf0` (`cursor/f08-bf004-stage2-fb90`)
BF-004 repair commit: `ce690772` — Fix BF-004 stale Note Off silencing current MIDI owner
Engine blob: `ab6d9667e9cd40ae12cc8c8ece8d7bb12be29216` (matches implement tip)
Audited / validated source SHA: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Content tree (pre-repair package baseline): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`
Git tree (pre-repair): `4dc57f74141fd1f01728a0001025b6e76065bf19`
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001)
Test run subagent: `bc-d9a62eaf-b661-5df9-9a5c-893e387d4a33`
Failure investigation subagent: not started (no failing required check)
No real MIDI opened. No device written. No source patch by this stage.
No GitHub Actions used as tests.
No source repair found; do not return to stage 2.

## Required BF-004 regressions

| Check | Command | Exit | Result | Log |
| --- | --- | ---: | --- | --- |
| host build | `sh host/build_host.sh` | 0 | PASS | `evidence/f08-stage3/build_host.log` |
| midiowner | `build/host/x0x_host tests/scenarios/midiowner.x0x build/scenarios/midiowner` | 0 | PASS | `evidence/f08-stage3/midiowner.log` |
| usbnotes | `build/host/x0x_host tests/scenarios/usbnotes.x0x build/scenarios/usbnotes` | 0 | PASS | `evidence/f08-stage3/usbnotes.log` |
| blenotes | `build/host/x0x_host tests/scenarios/blenotes.x0x build/scenarios/blenotes` | 0 | PASS | `evidence/f08-stage3/blenotes.log` |
| engine Note Off gate | code review + `git rev-parse HEAD:firmware/src/app/engine.c` | 0 | PASS | `evidence/f08-stage3/engine-noteoff-gate.txt` |

Machine summary: `evidence/f08-stage3/test-run-summary.json`. Full run: Project store `internal/f08-stage3-test-run.md`. Pillow install helper log: `evidence/f08-stage3/agent_env_install.log`.

## midiowner coverage (all ok)

- stale non-owner Off leaves 303 / BREAK hold
- owner Off / different-note / same-owner / TRS release green
- reverse ownership and USB/BT takeover paths covered by scenario

## Failures

None. Every required check met its expected exit. No cause/disposition rows.

## Overall

**ALL_PASS** — required BF-004 host regressions. Deferred items unchanged: `evidence/f08-stage2/DEFERRED.md`. BF-005 / BF-006 / BF-007 not taken.
