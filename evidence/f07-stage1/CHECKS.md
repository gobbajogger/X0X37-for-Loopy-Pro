# Stage 1 checks for F07 (audit of 1325f5d3)

Branch: `cursor/f07-fp-pin-stage1-fb90`
Stage-1 owner: `bc-b1215ac4-b3bd-5a72-a25e-e97bb3d629dd`
Validated **source** SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Catalog git tree: `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e`
Repair commits in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003)
Prior audited SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`
Evidence base tip: remote `cursor/f06-bf003-stage7-fb90` (includes owner-visible Stage-1 handoff record)
Independent claim review (internal Task): `bc-4fb732c3-0749-525e-91d0-f846466c1b04` — **13/13 PASS**
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-215.md` — Overall **PASS**

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-1325f5d3-dev` |
| `source_commit` | `1325f5d34340ea877e65ab86f3663f5dea052c0d` |
| `source_tree` | `6b6401f5c57ed4142bce1adbc3ceab2f99a80b6e` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89…2278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d9093211…fecb` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `f917b636…` |
| `gh-pages` tip | `cff694f3792d196267c2dc8ce2d86c6aef7d5e9e` |
| BF-003 / catalog markers | `1325f5d3` + `BF-003` + `nightly-1325f5d3-dev` **present** |
| Served install HTML sha256 | `66b56ba5accd1309664320c91db52c28438080a97b2bdb3fa31f6b830f3ca554` |
| Qualification source FP `8caf8724…` | **MISMATCH** vs served HTML (selected defect FP-001) |
| Nightly `installerFingerprint()` | `8caf8724…` **equals** qualification pin |

Log: `evidence/f07-stage1/live-verify.log`.

## Defect selection

| Step | Result |
| --- | --- |
| BF-001 already in `1325f5d3` | **YES** (`d11b74fc`) |
| BF-002 already in `1325f5d3` | **YES** (`ab90f47c`) |
| BF-003 already in `1325f5d3` | **YES** (this SHA; `node web/test_web.mjs` exit 0) |
| F03 already in `1325f5d3` | **YES** (`4f18983a`) |
| Withheld asset cache / data-safety / IS_OTA / longer SLOT_PROBE | Excluded / design / optional (prior audits) |
| Oldest deferred still open | **FP-001** Stage-7 installer FP pin ≠ served HTML |
| Qual pin ≠ live HTML reproduced | **YES** (`8caf8724…` ≠ `66b56ba5…`) |
| Recorded non-Actions package FAIL | **None** (packages MATCH; FP pin ≠ package FAIL) |

## Overall

**AUDIT RECORDED.** Selected defect **FP-001** (dual-digest reconciliation). Owner-visible Stage 2 successor `bc-d180cf42-3a0d-5bf5-b694-aa94e5cf04ef` recorded on HANDOFF.
