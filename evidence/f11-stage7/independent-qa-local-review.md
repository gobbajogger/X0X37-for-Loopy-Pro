# Independent QA for F11 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-4f96b0d9-dev`.

**Why this receipt applies:** BF-007 changed the firmware application tree (`bcc21733…` → `5b1a21f0…`) and content tree (`1da3bbfb…` → `f27c89e1…`), so Elite/Pro package SHA-256 **differ** from F10/F09 page digests. Stage-4 DEV CHECKED on validated source `4f96b0d9…` / content-tree `f27c89e1…` was locally reviewed on PR #264 (Overall **PASS**, digests MATCH). Stage-7 rebuild reproduced the same blank hashes and joined bundled installable bytes; HTTP re-fetch of hosted bundled packages MATCH.

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4 / stage-7 rebuild) | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` |
| Blank Pro (stage-4 / stage-7 rebuild) | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` |
| Page Elite (bundled) | `4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a` |
| Page Pro (bundled) | `b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Stage-3 `CHECKS.md` covers required BF-007 host regressions only and is **not** used alone for `independent-qa`.

**Primary local review (Overall PASS):** Project store `internal/pr-local-reviews/pr-264.md` (reviewer `bc-d2140e52-d624-573a-be0f-d8b2b20c820c`). Stage-7 rebuild also verified both models `DEV CHECKED` at `4f96b0d9…` before bundling (`tools/add_ctl.py`). Helper report: `internal/f11-stage7-package-rebuild.md`.

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.
