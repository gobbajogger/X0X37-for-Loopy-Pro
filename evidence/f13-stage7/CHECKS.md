# Stage 7 publish for F13 BF-012

Branch: `cursor/f13-bf012-stage7-237f`
Stage-7 owner: `bc-5bd25afb-db89-51d1-90c1-0c0a985ff2eb`
Audited / validated source SHA (pre-BF-012): `de11b6d7e3ac3322d2db689675d9ed27ed667785`
Validated **source** SHA (publish this candidate): `b42bcba46ee7255c18f18c10dd1f99b9fc9bb675`
Content tree (DEV CHECKED, non-`.md`): `519ccdf21b46db9198993587cb9334960e67ce483641c744c5caf71b1c769141`
Catalog `source_tree` (git): `a15afc439fc8b7bccd62d50403b925add45ae9e5`
Firmware tree: `5facb3b4405d119a269a05224b9268adfa1baa13`
BF-012 repair commit: `6636cca1`
Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008)

## Installer digest (local host checks only)

| Field | Value |
| --- | --- |
| `installer_sha256` / source fingerprint | `c039e6563a704d550428b7b1bc066c0e400b8b5ac97a7596a84e3bddd55b224a` |
| Note | Dual-digest fields kept distinct (FP-001). Fingerprint matches current nightly main; historical second qualification field / “Installer FP” wording rewrite remains deferred; not reopened. |
| Nightly `npm test` (clean main) | **PASS** (`nightly-npm-test.log`) — synthetic suites are **not** candidate QA for safety gates |
| `prepare:candidate` | **PASS** (HTTP package bytes re-fetched; size + SHA-256 match) |

GitHub Actions / Codex-on-GitHub not used as merge gate. Local-env review is the review gate.

## Independent QA (package digests)

| Field | Value |
| --- | --- |
| Receipt | `evidence/f13-stage7/independent-qa-local-review.md` |
| Primary | Stage-4 DEV CHECKED on `b42bcba46ee7…` (PR #302); Stage-7 digest verify + HTTP re-fetch MATCH |
| Bytes | Stage-4 blank MATCH; bundled join for page; HTTP re-fetch MATCH |
| Not used alone | `evidence/f13-stage3/CHECKS.md` (BF-012 host only) |

## Safety gates

| Gate | Status | Evidence |
| --- | --- | --- |
| `backup-restore-readback` | PASS | `safety-gates-note.md` + `check-safety-gates.py` / `safety-gates-candidate.log` — FakeX0X blank+`--ctl` on Elite `82ee29d3ede1…` / Pro `bc6c04229840…` join chain; CTL_READ CRC MATCH |
| `interruption-recovery` | PASS | same runner — mid-restore fail-closed exit 7; backup retained; both models |
| `ota-target-binding` | PASS | same runner — blank+page product identity Elite_900 / Pro_900 |

F12 byte-identity carry does **not** apply (BF-012 changed package digests). Nightly synthetic `npm test` is **not** candidate QA for these gates.

## Packages (DEV CHECKED blank; page serves bundled)

| Model | Blank sha256 (stage-4) | Page sha256 (bundled) | Size | Re-fetch |
| --- | --- | ---: | --- | --- |
| elite | `d9a9ccf4698d9519316d90b36f6158101f7f1c5cd96ba50f6c1be6ef8693ebcc` | `82ee29d3ede1c17427f2689fbcdd2ab7e2eca7c72df73dcd7694078c9e313b14` | 651396 | MATCH |
| pro | `a6a2387c712e95772ab1ff2f184f6b21357f71da08f0414f8d698b5f94309498` | `bc6c04229840c6342f6b8a696939bab7763a934b41d90a95f201250a42bfad34` | 651392 | MATCH |
| ctl | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | embedded | 65492 | embedded |

Immutable host: `artifacts/nightly-dev` @ `d8a03d45881ea0d26eedddcc30ffd71a94f42374` path `artifacts/nightly/dev-20261026-b42bcba4/<sha256>/…`. Manifest: `package-manifest.txt`.

## Pages deploy

| Field | Value |
| --- | --- |
| Push path | owner `gobbajogger` FF of nightly `gh-pages` (cloud cursor[bot] historically 403; this Mac/session has repo write) |
| Parent used for successful FF | `07baa4ead3ce99d40abb8aa60a47cf942257c1e9` |
| Live `gh-pages` tip | `0bfcbdf8fe3b204b3f6206ee634007ef814e32c1` (FF only; no force) |
| Live | **MATCH** `nightly-b42bcba4-dev` (`live-verify.log`) |
| Label | `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false |

## Production / deferred

Production `main` / production Pages unchanged. No device flash. Deferred: `evidence/f13-stage2/DEFERRED.md` / `evidence/f13-stage3/DEFERRED.md`. Firmware `nightly` remains `dcd288a4…` (FF refused at stage 5; no force-push). Next audit uses `b42bcba46ee7…`.

## Overall

**PUBLISHED LIVE** for validated source `b42bcba46ee7…` with matching package hashes, labeled hardware test / device not tested. Owner-visible Stage 1 successor pending coordinator CreateAgent (pod has no `CURSOR_API_KEY`).
