# Stage 1 checks for F13 (audit of de11b6d7)

Branch: `cursor/f13-stage1-audit-237f`
Stage-1 owner: `bc-09959f6e-dc18-5575-baa6-444dfdf698cc`
Validated **source** SHA: `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Git `source_tree`: `c143a9f8f5b5d0f6804995829ccbe515c77c6103`
Firmware tree: `9195dce5c4beb5fa8aa564504c8d1d114c8264cd`
Cross-check `origin/nightly`: `5e6d7e4ea387c42722d2704bdb43ef04f7caeda3` (ancestor includes `de11b6d7`; same `master.c` blob)
Repair commits in tip: BF-001…BF-008 + FP-001 + F02/F03 as listed in AUDIT.md
Prior published cycle: F12 BF-008 LIVE_PAGE_MATCH + Stage 6 PASS `e88f1ce2eb88ae4a`

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests. `X0X_TEST_JOBS=8` for host probes. `local_proof_plan` not required (audit tooling only).

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-de11b6d7-dev` |
| `source_commit` | `de11b6d7e3ac3322d2db689675d9ed27ed667785` |
| `source_tree` | `c143a9f8f5b5d0f6804995829ccbe515c77c6103` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `4841110a…d18a` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `b35d5875…c720` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| BF-008 / catalog markers | `de11b6d7` + `BF-008` + `nightly-de11b6d7-dev` **present** |
| Served install HTML sha256 | `8fcefff1f20bc52fc55375050cfb7cfffd1207dd86280d886537b6a0d7ae2950` |
| Result | **LIVE_PAGE_MATCH** |

Log: `evidence/f13-stage1/live-verify.log`.

## BF-012 witness (local host)

| Check | Result |
| --- | --- |
| Tip `master.c` blob | `b5557b6c41bdf45275547a3b6c5431d5252f62f0` |
| `origin/nightly` `master.c` blob | `b5557b6c…` (same; unfixed) |
| Tip baseline `master` suite | **PASS** (`master-tip-baseline.log`) |
| Tip + PR #68 regression overlay | **FAIL** exit non-zero — mono/stereo stale peak `0.500000` (`bf012-witness.log`) |
| Scratch PR #68 `master.c` + regression | **PASS** (`bf012-scratch-clear.log`) — not committed |
| PR #68 repair tip | `99543b84…` blobs `f91a9473…` / `df2f13b3…` |
| Tip tree restored after probe | **YES** (`git status` clean for production paths) |

## Defect selection

| Step | Result |
| --- | --- |
| BF-001…BF-008 + FP-001 already in `de11b6d7` | **YES** |
| Deferred f12-stage2 items 1–7 | Excluded / design / optional / decision-open / not-required / docs debt — **no tip FAIL** |
| Recorded non-Actions package FAIL | **None** on validated SHA |
| Open PR #28 fastmath | Tip already fixed (BF-008) — not selected |
| Open PR #68 master limiter | **Selected as BF-012** |
| Oldest open tip FAIL / finding | **BF-012** (`master.c` blob `b5557b6c…` still unfixed) |
| Arp/Fx #279/#281 leftovers | **Not selected** (sole-owner; not tip FAIL driver here) |

## Overall

**AUDIT RECORDED.** Selected defect **BF-012** (limiter OFF→ON stale look-ahead). Ready for Stage 2. Handoff incomplete until coordinator CreateAgent returns an owner-visible Stage 2 id.
