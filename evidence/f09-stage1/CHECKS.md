# Stage 1 checks for F09 (audit of 8a74ff52)

Branch: `cursor/f09-bf005-stage1-fb90`
Stage-1 owner: `bc-2ac9852a-4eac-553f-906c-d7ecf1a4d46e`
Validated **source** SHA: `8a74ff52cba842c3f2d8365600286f94e6bccdf0`
Content tree: `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf`
Catalog git tree: `33d6950b0a378525f2a8b592672275625893239f`
Firmware tree: `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f`
Audited firmware content tree (pre-BF-004 baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004)
Prior audited SHA: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Evidence base tip: remote `cursor/f08-bf004-stage7-fb90` (includes Stage-7 LIVE_PAGE_MATCH + Stage-1 handoff prompt)
Independent claim review (internal Task): `bc-2c8dbc00-6019-5cab-bf1c-52d90930ef4f` — **12/12 PASS**
Deferred research helper: `bc-733122bc-0711-5a24-abd5-8d03e225ad6c`
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-234.md` — Overall **PASS**
Package local review cite: Project store `internal/pr-local-reviews/pr-232.md` — Overall **PASS**

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)
Catalog: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/versions.json (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-8a74ff52-dev` |
| `source_commit` | `8a74ff52cba842c3f2d8365600286f94e6bccdf0` |
| `source_tree` | `33d6950b0a378525f2a8b592672275625893239f` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `d7767beb…db93` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `b0cecbcf…c5fa0` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `82622c1f…` |
| `gh-pages` tip | `e296e8834aff9f9f358fc6f63360abbff83334d4` |
| BF-004 / catalog markers | `8a74ff52` + `BF-004` + `nightly-8a74ff52-dev` **present** |
| Served install HTML sha256 | `aec5dc78f6d4fe198b8e32e7939c7c0533a5c585f4eeb04adadc6fad78379b31` |
| Source fingerprint qualification `4d29566e…` | **distinct** from served HTML (FP-001 closed; do not reopen) |

Log: `evidence/f09-stage1/live-verify.log`.

## BF-005 witness (local host)

| Check | Result |
| --- | --- |
| Tip `sequencer.c` blob | `83b03f73139d8e679c94526c66b390b6e5bc2f39` (matches PR #26 baseline) |
| `seq_clk_drop` | Clears `clk_lock` only; does not clear `ext_pending` |
| Harness (PR #26 lane) vs tip | **FAIL** exit 1 — `old_pending=3` accelerates new BLE owner (`bf005-witness.log`) |
| Scratch clear `ext_pending` in owner drop | **PASS** exit 0 (`bf005-scratch-clear.log`) — not committed |
| BF-004 engine blob | `ab6d9667…` fixed; not reopened |

## Defect selection

| Step | Result |
| --- | --- |
| BF-001 already in `8a74ff52` | **YES** (`d11b74fc`) |
| BF-002 already in `8a74ff52` | **YES** (`ab90f47c`) |
| BF-003 already in `8a74ff52` | **YES** (`1325f5d3`) |
| F03 already in `8a74ff52` | **YES** (`4f18983a`) |
| FP-001 already in `8a74ff52` | **YES** (`6df0f81f`) |
| BF-004 already in `8a74ff52` | **YES** (`ce690772`) |
| Deferred f08-stage2 items 1–7 | Excluded / design / optional / decision-open / not-required / docs debt — **no tip FAIL** |
| Recorded non-Actions package FAIL | **None** |
| Oldest open bugfinder | **BF-005** (sequencer.c blob `83b03f73…` still matches finding baseline) |
| Younger BF-006 / BF-007 | Open on PR #26 lane; not selected |

## Overall

**AUDIT RECORDED.** Selected defect **BF-005** (queued old-source MIDI clocks accelerate a new source after disconnect). Ready for Stage 2.
