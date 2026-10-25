# Safety gates for F11 new package digests

**Applies to:** `backup-restore-readback`, `interruption-recovery`, and `ota-target-binding` for Elite/Pro page packages of `nightly-4f96b0d9-dev`.

BF-007 changed installable Elite/Pro package bytes vs F10 (`9cba1f3d…`/`5dda4eec…` → `4841110a…`/`b35d5875…`). F10 byte-identity carry-forward does **not** apply.

## Substantiation (F09-style for new digests)

| Gate | Evidence | Why it applies |
| --- | --- | --- |
| `package-integrity` / `model-controller` / `memory-flash` | `evidence/f11-stage4/CHECKS.md` | Elite/Pro DEV CHECKED; pool_free 8324; identities Elite_900/Pro_900 |
| `host-regressions` | `evidence/f11-stage3/CHECKS.md` | BF-007 ALL_PASS |
| `backup-restore-readback` / `interruption-recovery` | stage-4 CHECKS + unchanged installer fingerprint `4d29566e…` | Nightly main installer binding/readback suites PASS (`npm test`); `prepare:candidate` re-fetches and verifies bundled controller for the new digests |
| `ota-target-binding` | stage-3 CHECKS + same installer fingerprint | Installer OTA/target binding tests unchanged; package model identity checked on download |
| `independent-qa` | `evidence/f11-stage7/independent-qa-local-review.md` + PR #264 local PASS | Blank MATCH rebuild; HTTP MATCH for hosted bundled bytes |

**Not claimed:** hardware test, production promotion, or that nightly synthetic suites alone are firmware device QA. Local-env review only.
