# Independent QA for F13 Stage 7 package digests

**Gate:** `independent-qa` for Elite/Pro page packages served as `nightly-b42bcba4-dev`.

**Why this receipt applies:** BF-012 changed the firmware application tree and content tree (`9195dce5…` / audited pre-BF-012 → firmware-tree `5facb3b4405d119a269a05224b9268adfa1baa13` / content-tree `519ccdf21b46…`), so Elite/Pro package SHA-256 **differ** from F12 page digests. Stage-4 DEV CHECKED on validated source `b42bcba46ee7…` / content-tree `519ccdf21b46…` is on PR #302. Stage-7 verified both models `DEV CHECKED` at `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675` before bundling (`tools/add_ctl.py`); HTTP re-fetch of hosted bundled packages MATCH.

| Identity | Digest |
| --- | --- |
| Blank Elite (stage-4) | `d9a9ccf4698d9519316d90b36f6158101f7f1c5cd96ba50f6c1be6ef8693ebcc` |
| Blank Pro (stage-4) | `a6a2387c712e95772ab1ff2f184f6b21357f71da08f0414f8d698b5f94309498` |
| Page Elite (bundled) | `82ee29d3ede1c17427f2689fbcdd2ab7e2eca7c72df73dcd7694078c9e313b14` |
| Page Pro (bundled) | `bc6c04229840c6342f6b8a696939bab7763a934b41d90a95f201250a42bfad34` |
| Controller | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Stage-3 `CHECKS.md` covers required BF-012 host regressions only and is **not** used alone for `independent-qa`.

**Primary local review:** Stage-4 records `evidence/f13-stage4/elite/record.json` and `pro/record.json` (DEV CHECKED, pool_free 8324). Stage-7 local digest verify: trees MATCH validated; blank hashes MATCH stage-4; join MATCH page digests; HTTP re-fetch MATCH. Helper: `package-manifest.txt`.

**Safety gates:** `backup-restore-readback`, `interruption-recovery`, and `ota-target-binding` are substantiated for these exact page digests by `evidence/f13-stage7/safety-gates-note.md` and `safety-gates-candidate.log`.

Policy: local-env review only — not GitHub Actions, not Codex-on-GitHub.
