# Stage 1 checks for F08 (audit of 6df0f81f)

Branch: `cursor/f08-bf004-stage1-fb90`
Stage-1 owner: `bc-78e96d95-98c8-5fe4-b4f0-287bc2ad068d`
Validated **source** SHA: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Content tree: `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`
Catalog git tree: `4dc57f74141fd1f01728a0001025b6e76065bf19`
Audited firmware content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001)
Prior audited SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Evidence base tip: remote `cursor/f07-fp-pin-stage7-fb90` (includes Stage-7 LIVE_PAGE_MATCH + Stage-1 handoff prompt)
Independent claim review (internal Task): `bc-5b578719-45a1-5aa8-9ae1-25087ce3609e` — **12/12 PASS**
Deferred research helper: `bc-cc5e637f-6d3f-5a99-9d08-759ddf037f08`
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-225.md` — Overall **PASS**
Package local review cite: Project store `internal/pr-local-reviews/pr-222.md` — Overall **PASS**

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)
Catalog: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/versions.json (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-6df0f81f-dev` |
| `source_commit` | `6df0f81fbbe0adc3207f67d912d2fe0ab676be04` |
| `source_tree` | `4dc57f74141fd1f01728a0001025b6e76065bf19` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89…2278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d9093211…fecb` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `f917b636…` |
| `gh-pages` tip | `d6a237c43cb6830d8cf0cb51e567181821630a28` |
| FP-001 / catalog markers | `6df0f81f` + `FP-001` + `nightly-6df0f81f-dev` **present** |
| Served install HTML sha256 | `d0866927e2b5baccfc3b2cde422c3f4eea2101a930cafd4de1c77ffd681cb54d` |
| Source fingerprint `8caf8724…` | **distinct** from served HTML (FP-001 closed; do not reopen) |

Log: `evidence/f08-stage1/live-verify.log`.

## Defect selection

| Step | Result |
| --- | --- |
| BF-001 already in `6df0f81f` | **YES** (`d11b74fc`) |
| BF-002 already in `6df0f81f` | **YES** (`ab90f47c`) |
| BF-003 already in `6df0f81f` | **YES** (`1325f5d3`) |
| F03 already in `6df0f81f` | **YES** (`4f18983a`) |
| FP-001 already in `6df0f81f` | **YES** (this SHA) |
| Deferred f07-stage2 items 1–7 | Excluded / design / optional / decision-open / not-required / docs debt — **no tip FAIL** |
| Recorded non-Actions package FAIL | **None** |
| Oldest open bugfinder | **BF-004** (engine.c blob `555480d9…` still matches finding baseline) |
| Younger BF-005 / BF-006 / BF-007 | Open on PR #26 lane; not selected |

## Overall

**AUDIT RECORDED.** Selected defect **BF-004** (stale Note Off interrupts current MIDI owner). Ready for Stage 2.
