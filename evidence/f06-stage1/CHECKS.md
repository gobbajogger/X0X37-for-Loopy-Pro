# Stage 1 checks for F06 (audit of ab90f47c)

Branch: `cursor/f06-bf003-stage1-fb90`
Stage-1 owner: `bc-a76a9628-9a50-5b04-a7a4-ba134f41d9e1`
Validated **source** SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`
Content tree: `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`
Catalog git tree: `50c381c528cae9692deff4a712071f7dc6acb020`
Repair commits in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03)
Prior audited SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`
Evidence base tip: `7288a4edfc03c20f64cbc6d2e0ec6e70e7a61555` (`cursor/f05-bf002-stage7-fb90`)
Independent claim review (internal Task): `bc-8ba88495-0eb2-5307-ad2a-c3f03dc65d6a` — all five claims **PASS**

No production source edited. No flash. No firmware `nightly` / `main` force-push. GitHub Actions were not used as tests.

## Live nightly page

URL: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/ (HTTP 200)

| Check | Result |
| --- | --- |
| `latest` | `nightly-ab90f47c-dev` |
| `source_commit` | `ab90f47ccad0f213114d6a98c1bc6179be2ebb35` |
| `source_tree` | `50c381c528cae9692deff4a712071f7dc6acb020` |
| label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` |
| `hardware_tested` | `false` |
| Elite bundled re-fetch | size 651396, sha256 `8ba23a89…2278` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `d9093211…fecb` **MATCH** |
| `controller_sha256` | `956e3e21…2d0a` (bundled) |
| Host | `artifacts/nightly-dev` @ `f917b636…` |
| `gh-pages` tip | `e10823e38ec9e56f5ec381822b1597f7364a95d9` |
| BF-001 / catalog markers | `d11b74fc` + `selection generations` + `nightly-ab90f47c-dev` **present** |
| Live install HTML sha256 | `0439c05891eb804ceb94c626a8eba919926f11d7454a81e706197f1466bde28b` |
| Stage-7 FP pin `2240be87…` | **MISMATCH** vs tip HTML (evidence pin only) |

Log: `evidence/f06-stage1/live-verify.log`.

## Defect selection

| Step | Result |
| --- | --- |
| BF-001 already in `ab90f47c` | **YES** (`d11b74fc`) |
| BF-002 already in `ab90f47c` | **YES** (this SHA; project suites selected) |
| F03 already in `ab90f47c` | **YES** (`4f18983a`) |
| Oldest deferred not fixed | **BF-003** (stale `web/test_web.mjs` → missing `editor.html`) |
| BF definition | Open PR #26 / `docs/audit/bugfinder-watch-2026-10-23.md` |
| Tip still ENOENT on `node web/test_web.mjs` | **YES** (exit 1; repro log) |
| Retire-versus-restore | **Retire approved** by this audit (intentional X0X deletion `23775375`; suite unregistered) |
| Recorded non-Actions FAIL | **None** outside BF-003 itself (Stage 3 ALL_PASS; Stage 4 DEV CHECKED; Stage 5 nightly FF REFUSED ≠ FAIL; FP pin ≠ package FAIL) |

## Overall

**AUDIT RECORDED.** Selected defect **BF-003** (retire). Ready for Stage 2.
