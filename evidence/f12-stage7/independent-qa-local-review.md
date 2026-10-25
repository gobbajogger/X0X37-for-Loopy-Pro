# Independent QA for F12 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-de11b6d7-dev`.

**Why this receipt applies:** BF-008 changes the firmware application tree (`5b1a21f0…` → `9195dce5…`) and content tree (`f27c89e1…` → `43e97f34…`) via a one-line defined unsigned cast in `fm_exp2_nf`. Stage-4 DEV CHECKED on validated source `de11b6d7…` / content-tree `43e97f34…` recorded blank package SHA-256 identical to F11 (`829bb6da…` / `d2186cb0…`) — identical object code under Clang 4.0.1 for that cast. Stage-4 was locally reviewed on PR #272 (Overall **PASS**, digests MATCH). Stage-7 re-fetches the immutable hosted bundled installable bytes and requires size + SHA-256 match before the page can offer them.

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4) | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` |
| Blank Pro (stage-4) | `d2186cb0f3fe74376651cb2804973c8255780532a8cd54c33d2bac09aeda60ec` |
| Page Elite (bundled; same immutable host as F11) | `4841110a274e2212d56abf9234cf0d921b45d915d1ff25fad0727e52edb7d18a` |
| Page Pro (bundled; same immutable host as F11) | `b35d587557d9f1d4c72b2c81bc944a1dd129bee625e98ae35f06fe7f653fc720` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Stage-3 `CHECKS.md` covers required BF-008 host regressions only and is **not** used alone for `independent-qa`.

**Primary local review (Overall PASS — Elite/Pro DEV CHECKED digests MATCH):** Project store `internal/pr-local-reviews/pr-272.md` (reviewer `bc-8cdb733e-5328-558a-a11a-5a3f3d04ca92`). Stage-7 HTTP re-fetch of page packages MATCH expected bundled digests before `prepare:candidate` (`http-refetch.log`). Blank packages extracted from those hosted bytes via `add_ctl.py --remove` MATCH stage-4 blank SHA-256.

**Catalog note:** `nightly-4f96b0d9-dev` is superseded in the flashable picker because the same package digests cannot carry two `source_commit` values in qualification. Older non-colliding versions are retained.

**Safety gates:** `backup-restore-readback`, `interruption-recovery`, and `ota-target-binding` are **not** attributed to F12 stage-3/4 BF-008-only receipts. They carry forward F11 qualification of these exact page digests via `evidence/f12-stage7/safety-gates-byte-identity.md`. Nightly synthetic `npm test` suites are not candidate QA for those gates.

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.
