# Stage 1 checks for F14 (audit of b42bcba4)

Branch: `cursor/f14-bf013-stage1-237f`
Stage-1 owner: `bc-4c124351-a5c1-53ea-a243-b3b478f7bc15`
Validated **source** SHA: `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Git `source_tree`: `a15afc439fc8b7bccd62d50403b925add45ae9e5`
Content tree (non-`.md`): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`
Firmware tree: `5facb3b4405d119a269a05224b9268adfa1baa13`
Cross-check `origin/nightly`: `bd5b7236…` (contains BF-013 PR #74; audited published SHA does not)
Repair commits in tip: BF-001…BF-008 + FP-001 + F02/F03 + BF-012 as listed in AUDIT.md
Prior published cycle: F13 BF-012 LIVE_PAGE_MATCH `nightly-b42bcba4-dev`

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests. `X0X_TEST_JOBS=8` for host probes. `local_proof_plan` not required (audit tooling only).

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-b42bcba4-dev` |
| `source_commit` | `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675` |
| `source_tree` | `a15afc439fc8b7bccd62d50403b925add45ae9e5` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `82ee29d3…131b14` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `bc6c0422…bfad34` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| BF-012 / catalog markers | `b42bcba4` + `BF-012` + `nightly-b42bcba4-dev` **present** |
| Served install HTML sha256 | `0b8bf63355feebb115c3c6ba072fa9645237f109a87d4f161b5b9086a908f8a1` |
| Result | **LIVE_PAGE_MATCH** |

Log: `evidence/f14-stage1/live-verify.log`.

## BF-013 witness (local host)

| Check | Result |
| --- | --- |
| Tip `fxbus.c` blob | `4c7485391d3a462ebe9f4907abfaf4867ce80b54` |
| Tip baseline `stereo` suite | **PASS** (`stereo-tip-baseline.log`) |
| Tip + PR #74 regression overlay | **FAIL** exit non-zero — four 20 BPM assertions (`bf013-witness.log`) |
| Scratch PR #74 `fxbus.c` + regression | **PASS** (`bf013-scratch-clear.log`) — not committed |
| PR #74 repair blobs | `fxbus.c` `49a89483…` / `stereo_test.c` `c8bc1092…` |
| Tip tree restored after probe | **YES** (`git status` clean for production paths) |

## Defect selection

| Step | Result |
| --- | --- |
| BF-001…BF-008 + BF-012 + FP-001 already in `b42bcba4` | **YES** |
| Deferred f13-stage2 items | Excluded / design / optional / decision-open / not-required / docs debt — **no tip FAIL** |
| Recorded non-Actions package FAIL | **None** on validated SHA |
| Open BF-013 delay 20 BPM on audited tip | **Selected** (`fxbus.c` blob `4c748539…` still unfixed on published SHA) |
| Younger BF-014+ already on `origin/nightly` | Not selected ahead of BF-013 on published tip |
| Arp/Fx #279/#281 leftovers | **Not selected** (sole-owner `bc-8efb2f94…`) |

## Overall

**AUDIT RECORDED.** Selected defect **BF-013** (delay sync at exact 20 BPM). Ready for Stage 2. Handoff incomplete until coordinator CreateAgent returns an owner-visible Stage 2 id (`CURSOR_API_KEY` unset in this pod).
