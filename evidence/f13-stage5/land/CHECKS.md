# Stage 5 land attempt — BF-012 onto current nightly

Branch: `cursor/f13-bf012-stage5-land-237f`
Stage-5 owner: `bc-77b4a99f-7d71-5957-ac89-ccaa9428016a`
Prior refusal evidence: `evidence/f13-stage5/` (FF of `b42bcba4` REFUSED; PR #306 records that only — **not** product Done).

## Land tip

| Field | Value |
| --- | --- |
| Land tip | `f4284ab8b622aa9d41ca26c3af9b79b38aeb4fca` |
| Parent | `origin/nightly` @ `bd2d99332e1f7abc3db3c8dd5839132d99e21e8f` (#295/#296) |
| Method | `git cherry-pick -x 6636cca1` (no force-push) |
| `master.c` blob | `f91a9473…` (matches Stage-2 repair) |
| `master_test.c` blob | `df2f13b3…` (matches Stage-2 repair) |

## Host proof (land tip)

| Check | Exit | Result |
| --- | ---: | --- |
| tip blobs vs BF-012 repair | 0 | PASS |
| `X0X_TESTS=master` `X0X_DEV_PADCAL=1` | 0 | PASS — `ok master` |

Logs: `blob-check.log`, `master-host.log`, `blob-confirm.txt`.

## Mini Stage-4 package revalidate (required — tip ≠ `b42bcba4`)

Workflow: tip change invalidates prior DEV CHECKED records. Ran Linux native `tools/dev_validate.py` / `./build.sh` with `JIELI_TOOLCHAIN` + `AC79_SDK` installed in this cloud VM.

| Commit | Role | Elite build | Note |
| --- | --- | --- | --- |
| `b42bcba4` | prior validated package tip | **PASS** (`code_free` 6564; package 651396 B) | Confirms toolchain works; Stage-4 evidence still applies only to this SHA |
| `bd2d9933` | current `origin/nightly` alone | **FAIL** XIP overflow **972 B** | Nightly tip unbuildable before BF-012 |
| `f4284ab8` | land tip (nightly + BF-012) | **FAIL** XIP overflow **1036 B** | +BF-012 grows overflow; no DEV CHECKED |

Logs: `nightly-only-elite-build.log`, `land-tip-elite-build.log`, `b42-elite-build.log`, `elite-validate.log`, `BLOCKER.txt`.

## Merge decision

**BLOCKED.** No Elite/Pro DEV CHECKED on land tip. Did **not** merge into `nightly`. Did **not** force-push. Host PASS alone is not a package PASS.

## Nightly SHA result

`origin/nightly` remains `bd2d99332e1f7abc3db3c8dd5839132d99e21e8f` (BF-012 **not** landed).

## Next (coordinator)

1. Restore ≥1036 B XIP headroom on `nightly` (or revert/trim the size growth from #295/#296 / other tip commits) — likely RAM/flash size Stage-2.
2. Re-cherry-pick BF-012 onto the buildable tip, re-run host + Elite/Pro DEV CHECK, then merge to `nightly`.
3. Only then: Stage-6 Mac install prompt for the **landed** SHA + Stage-7 CreateAgent.

Stage 6 / Stage 7 CreateAgent **deferred** until a landed nightly SHA exists with DEV CHECKED packages.
