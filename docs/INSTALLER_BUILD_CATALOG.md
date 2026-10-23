# X0X37 installer build inventory (development only)

Source of truth: `data/installer-builds.json`. This catalog describes **actual binaries**, not git branches, PRs or commits that happen to be newer. `latest` means newest **available DEV CHECKED package**, never latest commit/release/known-good device firmware. Current GitHub Releases collection is empty. No release candidate is implied.

## Labels

- **LATEST AVAILABLE DEV**: newest recoverable development package pair with recorded source, identities, SHA-256 and DEV CHECKED evidence; not a release.
- **DEV CHECKED**: passed model-specific host/size/integrity development validation; not physically flashed or validated on-device.
- **HARDWARE UNTESTED**: local physical install, reconnect, BLE SMC reception, backup/readback and performance checks remain pending.
- **SUPERSEDED**: archived valid previous package generation; exists for bisect/rollback evidence but is not recommended for routine installs.
- **SOURCE ONLY / EVIDENCE ONLY / NOT HOSTED / BUILD FAILED**: no install action and no implied artifact availability.
- **SMC ctl.img external**: do not ship third-party controller firmware or vendor SDK. For direct BLE, the user must supply a separate locally built compatible controller image through the install page.

## Safeguards

Published development packages are raw, immutable `.fwsc` Git objects. The website checks the **full SHA-256** against the catalog and the exact embedded model identity before enabling installation. A new selection requires explicit acknowledgment of untested DEV firmware; no package without a recorded SHA is automatically flashable. This is **not** a substitute for guarded local install evidence, sample backup/readback, model selection, flash/RAM budgets and hardware tests. Releases require separate owner authorization and release gates.

The two recovered package pairs are from separate validated revisions, not renamed copies. GitHub `gh-pages/firmware/` contains exact content-addressed copies of the tracked binaries; their hashes must continue to match the catalog. Never point a catalog entry at an updated source commit while reusing a previous binary/record. Old package archives with only textual evidence appear in `history` without download buttons.

Publish the existing GitHub Pages **site only**, not a GitHub Release. Do not commit user calibration, sample dumps, JieLi SDK/compiler/vendor firmware, credentials or a binary `ctl.img`. When regenerating the site, merge `gh-pages` firmware before publishing; the generator aborts if any referenced package is missing.

## Catalog and generator regression safeguards — 2026-10-23

The site generator now refuses a package when its embedded model does not match the explicitly requested `--pro`, `--elite`, `--mke` or `--fm1` destination. Partial republishing of an existing version preserves unchanged model packages, while any new or changed model bytes revoke inherited approval and require fresh model-specific validation before the corresponding build can be selected for installation. Unverified new versions cannot become `latest` automatically.

When reusing `gh-pages/versions.json`, local package paths are normalized for the installer page; carried-forward files are checked against their recorded byte size and SHA-256 after copying. Different firmware bytes cannot overwrite a historical filename, and the generator refuses `--merge` directories equal to or contained inside the output directory. A shell site generated from the canonical source registry without archived package files intentionally fails closed.

Validation workflow: `.github/workflows/installer-regressions.yml` checks the Python package/catalog generator, Web MIDI sample-slot installation simulation, sample importer, and portable-project paths. It does not publish the site or build firmware.

**Outstanding repository-level hazard:** The pre-existing `.github/workflows/pages.yml` and `tools/publish_pages.sh` use force pushes to `gh-pages`. They must be reconciled by the responsible Pages/integration owner before any source PR merge triggers site regeneration; a green generator test alone does not authorize that deployment path. Preserve source-branch validation and keep public website updates non-forced.
