# F14 Stage 2 checks — BF-013 delay sync at 20 BPM

**Branch:** `cursor/f14-bf013-stage2-909f`  
**Parent audit tip:** `cursor/f14-bf013-stage1-237f` @ `440876e1`  
**Audited / published source SHA:** `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`  
**Config:** `X0X_DEV_PADCAL=1`, `X0X_TEST_JOBS=8`  
**Policy:** local host only — GitHub Actions are not proof.

## Repair blobs

| Path | Blob | Source |
| --- | --- | --- |
| `firmware/src/dsp/fxbus.c` | `49a89483e1c777a46758ed370973afb85f15274d` | PR #74 / `af67d575` |
| `tests/host/stereo_test.c` | `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39` | PR #74 / `31dd6388` |

Tip baseline (unrepaired) `fxbus.c` blob: `4c7485391d3a462ebe9f4907abfaf4867ce80b54` (`> 20.0f`).

## Host results (this Stage 2 owner)

| Check | Command | Result | Log |
| --- | --- | --- | --- |
| Red witness | unrepaired tip `fxbus.c` + BF-013 `stereo_test.c` cases | **FAIL** — four 20 BPM assertions | `bf013-red.log` |
| Green | repaired `fxbus.c` + cases | **PASS** `ok stereo` | `bf013-green.log` |
| Host unit | `X0X_TESTS="stereo master"` | **PASS** | `host-unit.log` |

Exact commands:

```sh
X0X_TEST_JOBS=8 X0X_TESTS=stereo X0X_DEV_PADCAL=1 sh tests/run_tests.sh
X0X_TEST_JOBS=8 X0X_TESTS="stereo master" X0X_DEV_PADCAL=1 sh tests/run_tests.sh
```

Red FAIL lines (expected):

- `fx tempo: supported minimum 20 BPM accepted`
- `fx tempo: 20 BPM dotted-eighth delay folds to 1125 ms`
- `fx tempo: subdivision change at 20 BPM remains synchronized`
- `fx tempo: below-minimum input rejected`

## Not run / out of scope

- Firmware Elite/Pro package rebuild — Stage 4
- Device flash — Stage 6
- Firmware `nightly` fast-forward — Stage 5
- GitHub Actions — ignored as proof

## Note on diverged nightly

`origin/nightly` already contains PR #74 merge `f62ff102` (same `>= 20.0f` intent; tip `fxbus.c` also has later pot-curve helper refactor). This Stage 2 re-applies the repair onto the **published** audit lineage (`b42bcba4`) so the Full Cycle can validate and publish that source. Do not import unrelated nightly-only history into this tip.
