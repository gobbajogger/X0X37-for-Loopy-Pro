# Independent QA for F09 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-d0d00da2-dev`.

**Why this receipt applies:** BF-005 changed the firmware application tree (`581d1cb2…` → `32a36f68…`), so Elite/Pro package SHA-256 **differ** from F08 page digests. Stage-4 DEV CHECKED on validated source `d0d00da2…` / content-tree `62339630…` was locally reviewed on PR #243 (Overall **PASS**, packages rebuilt + verified). Stage-7 rebuild reproduced the same blank hashes and joined bundled installable bytes.

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4 / stage-7 rebuild) | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` |
| Blank Pro (stage-4 / stage-7 rebuild) | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` |
| Page Elite (bundled) | `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11` |
| Page Pro (bundled) | `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Stage-3 `CHECKS.md` covers required BF-005 host regressions only and is **not** used alone for `independent-qa`.

**Primary local review (Overall PASS — rebuilt + verified packages):** Project store `internal/pr-local-reviews/pr-243.md` (reviewer `bc-6072e4aa-ecc3-5ea4-9518-8ab1608389e6`). Stage-7 rebuild also verified both models `DEV CHECKED` at `d0d00da2…` before bundling (`tools/add_ctl.py`). Rebuild helper: Project store `internal/f09-stage7-package-rebuild.md`.

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.

---

# Local review — PR #243 (excerpt)

| Field | Value |
| --- | --- |
| PR | https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/243 |
| Package validation commit | `d0d00da23c3ce2268f06f31f8c062716d5a37ad0` |
| Package content tree (non-`.md`) | `62339630bc6e133da464eed7924645c0bfc09e4de66a8bccafdc60fd2d660c83` |
| Git tree at validation commit | `e06299abf0f35f24c771d011d07c2e74de613369` |
| Firmware tree | `32a36f68d1ce7a3c304b447475afe7e2bbb116cf` |
| Repair under test | **BF-005** — owner-matching `seq_clk_drop` clears `ext_pending` |
| Sequencer blob | `fd3f65306df166494d10facf8ddae058287cc17b` |
| Overall | **PASS** — Elite/Pro `DEV CHECKED` reproduced locally; package/controller digests MATCH |

Blank Elite/Pro sha256 and controller sha256 above match that review and Stage-7 rebuild. Page hashes are the bundled join of those blank packages with the same `ctl.img`.
