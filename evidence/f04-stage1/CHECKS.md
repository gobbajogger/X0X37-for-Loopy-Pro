# Stage 1 checks for F04 (audit of a03cfab3)

Branch: `cursor/f04-bf001-stage1-fb90`
Stage-1 owner: `bc-b24569bd-4f94-5a38-abb3-ed5ce76d4012`
Validated **source** SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Content tree: `1755a2dd6d8b79e7405ab58919cc4d60a69ca11a4ae6601030b0f5de963f170e`
Catalog git tree: `85e159c5365b14c48e9f3df7d2089e8f2eb560d4`
Repair commit in tip: `4f18983a` (F03)
Prior audited SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`
Evidence base tip: `5830fbf0160730b506c8c6bf38a77547190426b1` (`cursor/f03-slot-probe-stage7-fb90`)

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-a03cfab3-dev` |
| `source_commit` | `a03cfab3a423ab5c65a6546afd4993fe51754ee7` |
| `source_tree` | `85e159c5365b14c48e9f3df7d2089e8f2eb560d4` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89…2278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d9093211…fecb` **MATCH** |
| Elite blank re-fetch | size 651396, sha256 `d3b4cac3…5f2b` **MATCH** |
| Pro blank re-fetch | size 651392, sha256 `0b09605f…76b9` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `f917b636…` |
| `gh-pages` tip | `4b3c5994eabd14aa1a1a575b2211800a478112b4` |

Log: `evidence/f04-stage1/live-verify.log`.

## Defect selection

| Step | Result |
| --- | --- |
| F03 already in `a03cfab3` | **YES** (`4f18983a`) |
| Oldest deferred not fixed | **BF-001** (local `.fwsc` selection race) |
| BF definition | Open PR #26 / `docs/audit/bugfinder-watch-2026-10-23.md` |
| Tip still has race | **YES** (`web/x0x_installer.html` acceptStock / stockfile / drop; no generation token) |
| Draft fix PR #31 in tip ancestry | **NO** |
| Recorded non-Actions FAIL | **None** (Stage 3 ALL_PASS; Stage 4 DEV CHECKED; Stage 5 nightly FF REFUSED ≠ FAIL) |

## Overall

**AUDIT RECORDED.** Selected defect **BF-001**. Ready for Stage 2.
