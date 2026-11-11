# Independent QA for F10 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-af494784-dev`.

**Why this receipt applies:** BF-006 changes the firmware application tree (`32a36f68…` → `bcc21733…`) via a one-line defined cast in `d8_exp2_cr`. Stage-4 DEV CHECKED on validated source `af494784…` / content-tree `1da3bbfb…` recorded blank package SHA-256 identical to F09 (`de0bb077…` / `25bebb26…`) — identical object code under Clang 4.0.1 for that cast. Stage-4 was locally reviewed on PR #253 (Overall **PASS**, digests MATCH). Stage-7 re-fetches the immutable hosted bundled installable bytes and requires size + SHA-256 match before the page can offer them.

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4) | `de0bb0773c0b6de17140b180f79ae0e157c1690fcc485e6ffba28ae299eebc77` |
| Blank Pro (stage-4) | `25bebb261432e56b20f24b2100e05c369b9d3e8c277811ac300bae1050bd5e40` |
| Page Elite (bundled; same immutable host as F09) | `9cba1f3d0bebc2113d59b9bbb3f1ca1a67bb396183be279b74486319ddae8e11` |
| Page Pro (bundled; same immutable host as F09) | `5dda4eecd74d96237a3d457a026d7bc04eb3ed570705179ab8e6c15a198fa739` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Stage-3 `CHECKS.md` covers required BF-006 host regressions only and is **not** used alone for `independent-qa`.

**Primary local review (Overall PASS — Elite/Pro DEV CHECKED digests MATCH):** Project store `internal/pr-local-reviews/pr-253.md` (reviewer `bc-10a8bd45-2124-5d89-a969-38875b3fb2e7`). Stage-7 HTTP re-fetch of page packages MATCH expected bundled digests before `prepare:candidate`.

**Catalog note:** `nightly-d0d00da2-dev` is superseded in the flashable picker because the same package digests cannot carry two `source_commit` values in qualification. Older non-colliding versions are retained.

**Safety gates (Codex P1):** `backup-restore-readback`, `interruption-recovery`, and `ota-target-binding` are **not** attributed to F10 stage-3/4 BF-006-only receipts. They carry forward F09 qualification of these exact page digests via `evidence/f10-stage7/safety-gates-byte-identity.md`. Nightly synthetic `npm test` suites are not candidate QA for those gates.

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.

---

# Local review — PR #253 (excerpt)

| Field | Value |
| --- | --- |
| PR | https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/253 |
| Package validation commit | `af4947846df7233c68a6be72c4d0c634052a2752` |
| Package content tree (non-`.md`) | `1da3bbfbbf0df52256acceecbca2e064a1ef821478a33d78591e01813229643d` |
| Git tree at validation commit | `c12a7e747b79aa4619466764ecccc88285d4c10d` |
| Firmware tree | `bcc21733ea77097a787cfa6cc617fbdf1dca5da1` |
| Repair under test | **BF-006** — `d8_exp2_cr` defined exponent bias shift |
| drum808 blob | `90f463295fb1a642bad81700d73fc22eb8ec7a38` |
| Overall | **PASS** — Elite/Pro `DEV CHECKED`; package/controller digests MATCH |

Blank Elite/Pro sha256 and controller sha256 above match that review. Page hashes are the bundled join of those blank packages with the same `ctl.img` (immutable host `artifacts/nightly-dev` @ `2363c07f…`).
