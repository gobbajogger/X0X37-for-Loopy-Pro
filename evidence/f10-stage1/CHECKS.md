# Stage 1 checks for F10 (audit of d0d00da2)

Branch: `cursor/f10-bf006-stage1-fb90`
Stage-1 owner: `bc-6b7eb25f-60be-5d9e-8fa7-d8b9111b5e63`
Validated **source** SHA: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Content tree: `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83`
Catalog git tree: `e06299abf0f35f24c771d011d07c2e74de613369`
Firmware tree: `32a36f68d1ce7a3c304b447475afe7e2bbb116cf`
Audited firmware content tree (pre-BF-005 baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005)
Prior audited SHA: `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Evidence base tip: remote `cursor/f09-bf005-stage7-fb90` (includes Stage-7 LIVE_PAGE_MATCH + Stage-1 handoff prompt)
Independent claim review (internal Task): `bc-a4b5bfab-9e69-5adc-9677-9d2dfe23e20a` — **12/12 PASS**
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-245.md` — Overall **PASS**
Package local review cite: Project store `internal/pr-local-reviews/pr-243.md` — Overall **PASS**

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)
Catalog: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/versions.json (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-d0d00da2-dev` |
| `source_commit` | `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` |
| `source_tree` | `e06299abf0f35f24c771d011d07c2e74de613369` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `9cba1f3d…8e11` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `5dda4eec…fa739` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `2363c07f…` |
| `gh-pages` tip | `4f468d16c2a8c67be8817db0637d909b2fd3db85` |
| BF-005 / catalog markers | `d0d00da2` + `BF-005` + `nightly-d0d00da2-dev` **present** |
| Served install HTML sha256 | `472b486350b3a05a33a5459bcc95d705287a688c85ee460df60f84c48a0c31f9` |
| Source fingerprint qualification `4d29566e…` | **distinct** from served HTML (FP-001 closed; do not reopen) |

Log: `evidence/f10-stage1/live-verify.log`.

## BF-006 witness (local host)

| Check | Result |
| --- | --- |
| Tip `drum808.c` blob | `4b873a79cb050a789cfa26c2490ca8cf61c94ecb` (matches PR #26 baseline) |
| `d8_exp2_cr` ~165 | `b.i += (int32_t)n << 23;` (signed left shift) |
| Harness (PR #26 lane) vs tip + GCC UBSan | **FAIL** exit 1 — `left shift of negative value -3` (`bf006-witness.log`) |
| Scratch `b.u += (uint32_t)(int32_t)n << 23;` | **PASS** exit 0 (`bf006-scratch-clear.log`) — not committed |
| BF-005 sequencer blob | `fd3f6530…` fixed; not reopened |

## Defect selection

| Step | Result |
| --- | --- |
| BF-001 already in `d0d00da2` | **YES** (`d11b74fc`) |
| BF-002 already in `d0d00da2` | **YES** (`ab90f47c`) |
| BF-003 already in `d0d00da2` | **YES** (`1325f5d3`) |
| F03 already in `d0d00da2` | **YES** (`4f18983a`) |
| FP-001 already in `d0d00da2` | **YES** (`6df0f81f`) |
| BF-004 already in `d0d00da2` | **YES** (`ce690772`) |
| BF-005 already in `d0d00da2` | **YES** (`94f4a012`) |
| Deferred f09-stage2 items 1–7 | Excluded / design / optional / decision-open / not-required / docs debt — **no tip FAIL** |
| Recorded non-Actions package FAIL | **None** |
| Oldest open bugfinder | **BF-006** (drum808.c blob `4b873a79…` still matches finding baseline) |
| Younger BF-007 | Open on PR #26 lane; not selected |

Evidence PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/250
API CreateAgent attempts: `api-create-stage2-attempt1.json` / `attempt2.json` — **NO_CURSOR_API_KEY**

## Overall

**AUDIT RECORDED.** Selected defect **BF-006** (808 `d8_exp2_cr` signed left-shift UB). Ready for Stage 2. Handoff incomplete until coordinator returns owner-visible Stage 2 id.
