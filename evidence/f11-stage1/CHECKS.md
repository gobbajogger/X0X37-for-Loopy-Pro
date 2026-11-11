# Stage 1 checks for F11 (audit of af494784)

Branch: `cursor/f11-bf007-stage1-fb90`
Stage-1 owner: `bc-1751adc0-5254-5401-a54e-48e433f242b9`
Validated **source** SHA: `af4947846df7233c68a6be72c4d0c634052a2752`
Content tree: `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d`
Catalog git tree: `c12a7e747b79aa4619466764ecccc88285d4c10d`
Firmware tree: `bcc21733ea77097a787cfa6cc617fbdf1dca5da1`
Audited firmware content tree (pre-BF-005 baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006)
Prior audited SHA: `d0d00da23c3ce2268f06f31f8c062716d5a37ad0`
Evidence base tip: remote `cursor/f10-bf006-stage7-fb90` (includes Stage-7 LIVE_PAGE_MATCH + Stage-1 handoff prompt)
Independent claim review (internal Task): `bc-b123ad99-27b7-5044-b4c6-efd6eccf2b4e` — **8/8 PASS**
Research (internal Task): `bc-cc91a2a4-a49a-5c31-964b-ccda991e032e` — SELECT BF-007
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-256.md`
Package local review cite: Project store `internal/pr-local-reviews/pr-253.md` — Overall **PASS**

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)
Catalog: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/versions.json (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-af494784-dev` |
| `source_commit` | `af4947846df7233c68a6be72c4d0c634052a2752` |
| `source_tree` | `c12a7e747b79aa4619466764ecccc88285d4c10d` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `9cba1f3d…8e11` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `5dda4eec…fa739` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `2363c07f…` |
| `gh-pages` tip | `f45cae0956fba10218ed54d9b95ba5a8c7a4b63e` |
| BF-006 / catalog markers | `af494784` + `BF-006` + `nightly-af494784-dev` **present** |
| Served install HTML sha256 | `190a9a9ac18a30e6b4fd9091cdcda34cb4b65aeaa0d351368084012688bdb52e` |
| Source fingerprint qualification `4d29566e…` | **distinct** from served HTML (FP-001 closed; do not reopen) |
| Result | **LIVE_PAGE_MATCH** |

Log: `evidence/f11-stage1/live-verify.log`.

## BF-007 witness (local host)

| Check | Result |
| --- | --- |
| Tip `project.c` blob | `1000a0d88e04553295b82d724a63670434b67748` (matches PR #26 baseline) |
| `sound_bd_sends` call site | Before `load_stream` / `load_arrange1` (~549–556) |
| Harness (PR #26 lane) vs tip | **FAIL** exit 1 — 171 checks / 3 FAIL (`bf007-witness.log`) |
| Scratch migrate after arrange/stream load | **PASS** exit 0 — 171/171 (`bf007-scratch-clear.log`) — not committed |
| BF-006 drum808 blob | `90f46329…` fixed; not reopened |

## Defect selection

| Step | Result |
| --- | --- |
| BF-001 already in `af494784` | **YES** (`d11b74fc`) |
| BF-002 already in `af494784` | **YES** (`ab90f47c`) |
| BF-003 already in `af494784` | **YES** (`1325f5d3`) |
| F03 already in `af494784` | **YES** (`4f18983a`) |
| FP-001 already in `af494784` | **YES** (`6df0f81f`) |
| BF-004 already in `af494784` | **YES** (`ce690772`) |
| BF-005 already in `af494784` | **YES** (`94f4a012`) |
| BF-006 already in `af494784` | **YES** (`37bab7d1`) |
| Deferred f10-stage2 items 1–7 | Excluded / design / optional / decision-open / not-required / docs debt — **no tip FAIL** |
| Open PR #28 fastmath | Separate occurrence; **not selected** |
| Recorded non-Actions package FAIL | **None** |
| Oldest open bugfinder | **BF-007** (`project.c` blob `1000a0d8…` still matches finding baseline) |

Evidence PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/257
API CreateAgent attempts: `api-create-stage2-attempt1.json` / `attempt2.json` — **NO_CURSOR_API_KEY**

## Overall

**AUDIT RECORDED.** Selected defect **BF-007** (legacy BD PAN motion migration order). Ready for Stage 2. Handoff incomplete until coordinator returns owner-visible Stage 2 id.
