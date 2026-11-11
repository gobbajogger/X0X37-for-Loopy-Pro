# Stage 1 checks for F05 (audit of 0d79d634)

Branch: `cursor/f05-bf002-stage1-fb90`
Stage-1 owner: `bc-bd9cb2ca-4784-5d48-968d-bee42f593ff1`
Validated **source** SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`
Content tree: `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`
Catalog git tree: `603e2a2b1442a8214cd97b03d32bf57fc158feac`
BF-001 repair in tip: `d11b74fc`
F03 repair in tip: `4f18983a`
Prior audited SHA: `a03cfab3a423ab5c65a6546afd4993fe51754ee7`
Evidence base tip: `2a354e38a46891dda7e0d25da8a60a0bf8f9bff0` (`cursor/f04-bf001-stage7-fb90`)

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-0d79d634-dev` |
| `source_commit` | `0d79d63485dc6585800a71ef716d095445f5a1fc` |
| `source_tree` | `603e2a2b1442a8214cd97b03d32bf57fc158feac` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89…2278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d9093211…fecb` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `f917b636…` |
| `gh-pages` tip | `38b5d2a7d6380a4b9fd9ea1ff28d45b53d826569` |
| BF-001 markers | `d11b74fc` + `selection generations` **present** |
| Live install HTML sha256 | `3cd34559ee18de416171c0d6c51b914bf9e231f01cea3d98dd3e8b319af80145` |
| Stage-7 FP pin `2240be87…` | **MISMATCH** vs tip HTML (evidence pin only) |

Log: `evidence/f05-stage1/live-verify.log`.

## Defect selection

| Step | Result |
| --- | --- |
| BF-001 already in `0d79d634` | **YES** (`d11b74fc`) |
| F03 already in `0d79d634` | **YES** (`4f18983a`) |
| Oldest deferred not fixed | **BF-002** (scoped quickcheck omits `projects-web` / `projects-page`) |
| BF definition | Open PR #26 / `docs/audit/bugfinder-watch-2026-10-23.md` |
| Tip still omits project suites | **YES** (`tools/quickcheck_plan.py` `web/` route; repro log) |
| Recorded non-Actions FAIL | **None** (Stage 3 ALL_PASS; Stage 4 DEV CHECKED; Stage 5 nightly FF REFUSED ≠ FAIL; FP pin ≠ package FAIL) |

## Overall

**AUDIT RECORDED.** Selected defect **BF-002**. Ready for Stage 2.
