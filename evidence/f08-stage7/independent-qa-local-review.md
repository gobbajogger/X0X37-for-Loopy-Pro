# Independent QA for F08 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-8a74ff52-dev`.

**Why this receipt applies:** BF-004 changed the firmware application tree (`19840d87…` → `581d1cb2…`), so Elite/Pro package SHA-256 **differ** from F07/F06/F05/F04 page digests. Stage-4 DEV CHECKED on validated source `8a74ff52…` / content-tree `1a1e7a22…` was locally reviewed on PR #232 (Overall **PASS**, packages rebuilt + verified). Stage-7 rebuild reproduced the same blank hashes and joined bundled installable bytes.

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4 / stage-7 rebuild) | `aab3cd8e245dedaf561cd973b3ce05aac33eb84ba0077b9b81cb1d9aa3b260b4` |
| Blank Pro (stage-4 / stage-7 rebuild) | `066e2b1aeb6ed4abc8b3fba48606eaec1b42aa029a29ce07ffb0049ad7064a1e` |
| Page Elite (bundled) | `d7767bebf1413b5d6fb918ba3893335a7b9fae86078ce895aa3b72df48bedb93` |
| Page Pro (bundled) | `b0cecbcff42589cdc71231a16098b7cfc8301a8562aba25362d552b3dcdc5fa0` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Stage-3 `CHECKS.md` covers required BF-004 host regressions only and is **not** used alone for `independent-qa`.

**Primary local review (Overall PASS — rebuilt + verified packages):** Project store `internal/pr-local-reviews/pr-232.md` (reviewer `bc-555cf722-f4f9-5da3-87c3-cce18d514381`). Stage-7 rebuild also verified both models `DEV CHECKED` at `8a74ff52…` before bundling (`tools/add_ctl.py`).

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.

---

# Local review — PR #232 (excerpt)

| Field | Value |
| --- | --- |
| PR | https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/232 |
| Package validation commit | `8a74ff52cba842c3f2d8365600286f94e6bccdf0` |
| Package content tree (non-`.md`) | `1a1e7a22d136fef6e30702c9d3207d22798a6dd382717acc5ab0f40d2f235acf` |
| Git tree at validation commit | `33d6950b0a378525f2a8b592672275625893239f` |
| Firmware tree | `581d1cb213e6ceda3e9fbbcdc324ef8dce67c42f` |
| Repair under test | **BF-004** — ownership-gated Note Off / BREAK off for USB/Bluetooth |
| Engine blob | `ab6d9667e9cd40ae12cc8c8ece8d7bb12be29216` |
| Overall | **PASS** — Elite/Pro `DEV CHECKED` reproduced locally; package/controller digests MATCH |

Blank Elite/Pro sha256 and controller sha256 above match that review and Stage-7 rebuild. Page hashes are the bundled join of those blank packages with the same `ctl.img`.
