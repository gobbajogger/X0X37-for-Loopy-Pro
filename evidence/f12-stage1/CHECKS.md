# Stage 1 checks for F12 (audit of 4f96b0d9)

Branch: `cursor/f12-stage1-audit-1207`
Stage-1 owner: `bc-3fbb0761-3307-572f-a879-07db26c01207`
Validated **source** SHA: `4f96b0d9f5f11f700ea3fc935800b8714e11f35d`
Content tree: `f27c89e1ac8e11ab57c5ae7e49145ce19b964082bf3ca9a700a99bfa8a97091e`
Catalog git tree: `7b35932d5f0adbceeb35111f2136855aa54ebc9b`
Firmware tree: `5b1a21f07563b0262ece6728051cb0436bd4bfa1`
Audited firmware content tree (pre-BF-005 baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair commits in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007)
Prior audited SHA: `af4947846df7233c68a6be72c4d0c634052a2752`
Evidence base tip: remote `cursor/f11-bf007-stage7-179d` (includes Stage-7 LIVE_PAGE_MATCH + Stage-1 handoff prompt)
Research (internal Task): `bc-92c55624-604c-59b1-bd5b-2ed41ff7fa17` — SELECT BF-008
Witness (internal Task): `bc-2b17e9ee-82af-57da-93f7-92ac3ec18f94` — tip FAIL / scratch PASS
Stage-7 local review cite: Project store `internal/pr-local-reviews/pr-268.md`
Package local review cite: Project store `internal/pr-local-reviews/pr-264.md` — Overall **PASS**

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)
Catalog: embedded in install HTML (`versions.json` path returns 404; catalog inlined — F11 publish path)

| Check | Result |
| --- | --- |
| `latest` | `nightly-4f96b0d9-dev` |
| `source_commit` | `4f96b0d9f5f11f700ea3fc935800b8714e11f35d` |
| `source_tree` | `7b35932d5f0adbceeb35111f2136855aa54ebc9b` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `4841110a…d18a` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `b35d5875…c720` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `02f4f4f4…` |
| `gh-pages` tip | `48ffd27ebaceff48494cb3145c6699bb1a5c1def` |
| BF-007 / catalog markers | `4f96b0d9` + `BF-007` + `nightly-4f96b0d9-dev` **present** |
| Served install HTML sha256 | `f381efb6ed5eab50e530ea0fce56331d439efc39418350936f21c4cafdd286f3` |
| Source fingerprint qualification `4d29566e…` | **distinct** from served HTML (FP-001 closed; do not reopen) |
| Result | **LIVE_PAGE_MATCH** |

Log: `evidence/f12-stage1/live-verify.log`.

## BF-008 witness (local host)

| Check | Result |
| --- | --- |
| Tip `fastmath.h` blob | `7d2e41bf70590bafa8af0a7376bb912bf763b098` (matches unfixed PR #28 baseline) |
| Tip line | `b.i += n << 23;` in `fm_exp2_nf` (~57) |
| Minimal GCC UBSan vs tip | **FAIL** exit 1 — `left shift of negative value -3` (`bf008-witness.log`) |
| Scratch `b.u += (uint32_t)n << 23;` | **PASS** exit 0 — `fm_exp2f(-3) = 0.125` (`bf008-scratch-clear.log`) — not committed |
| Numeric `fastmath_test.c` (no UBSan) | **PASS** (`fastmath-numeric.log`) — not the witness |
| BF-007 `project.c` blob | `240a4b17…` fixed; host `legacy-bdpan-motion` 171/171 (`bf007-regression.log`) |
| PR #28 repair blob | `a5c26c95…` |

## Defect selection

| Step | Result |
| --- | --- |
| BF-001 already in `4f96b0d9` | **YES** (`d11b74fc`) |
| BF-002 already in `4f96b0d9` | **YES** (`ab90f47c`) |
| BF-003 already in `4f96b0d9` | **YES** (`1325f5d3`) |
| F03 already in `4f96b0d9` | **YES** (`4f18983a`) |
| FP-001 already in `4f96b0d9` | **YES** (`6df0f81f`) |
| BF-004 already in `4f96b0d9` | **YES** (`ce690772`) |
| BF-005 already in `4f96b0d9` | **YES** (`94f4a012`) |
| BF-006 already in `4f96b0d9` | **YES** (`37bab7d1`) |
| BF-007 already in `4f96b0d9` | **YES** (`78a0452e`) |
| Deferred f11-stage2 items 1–7 | Excluded / design / optional / decision-open / not-required / docs debt — **no tip FAIL** |
| Open PR #28 fastmath | **Selected as BF-008** |
| Recorded non-Actions package FAIL | **None** |
| Oldest open tip FAIL / finding | **BF-008** (`fastmath.h` blob `7d2e41bf…` still unfixed) |

Evidence PR: pending (this tip).
API CreateAgent Stage 2: pending coordinator (pod has no `CURSOR_API_KEY`).

## Overall

**AUDIT RECORDED.** Selected defect **BF-008** (`fm_exp2_nf` signed left-shift UB). Ready for Stage 2. Handoff incomplete until coordinator returns owner-visible Stage 2 id.
