# N1 / PR #903: cross-version legacy download alias retention

## Exact scope and source
- PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/903
- Parent head before this fix: `925294a3f9da7be2e82006bfdb4a15d4678858e2` (tree `ba0891fac2e07cc1f2a8a26fe4f2a926ed422605`).
- Production file: `tools/publish_nightly.py`, tested Git blob `b9dcec043e284f03620f1d3e8c721a6cec65f915`.
- Regression file: `tests/host/test_release_publish.py`, tested Git blob `6f8cc8132325e5fe24d4ab740892076312eef016`.
- Previous same-version regression 8/8 at `925294a3` is retained, not replaced. One existing test was extended, not a duplicate suite.

## Problem reproduced
At `925294a3`, `_keep_checked_numeric_aliases()` only considered aliases for the incoming version. A migrated `0.1.3` catalog advertised canonical DnBOX names, while legacy files were no longer referenced by `versions.json`. When generating `0.1.4`, `make_pages.write_site()` retained only catalog-referenced files and the new `0.1.4` packages, deleting both legacy `0.1.3` URLs.

The expanded `test_old_generator_to_new_numeric_republish_keeps_checked_urls` exercised real package verification, catalog creation and site generation with synthetic Elite/Pro package fixtures. The actual old generator filename expressions were injected only for the first `0.1.3` generation.

**RED** with original publisher blob `0ef1efd84b8bea872f9afd91b03876777d44dbc9`, new test blob `6f8cc8132325e5fe24d4ab740892076312eef016`:
`PYTHONDONTWRITEBYTECODE=1 python3 -m unittest tests/host/test_release_publish.py -k old_generator_to_new_numeric_republish_keeps_checked_urls`
returned 1 ERROR, exit 1: missing `after-0.1.4/firmware/x0x37-0.1.3-elite.fwsc`.
Raw log: `n1-pr903-cross-version-RED.log` (Git blob `fc7595b682026c25e3911827dd56c2658584810c`, SHA-256 `7a59f0de37f3998a0c3adeac7ab2d446507eadc09c07696cea446530b3ce924f`).

## Focused fix
A per-model `compat_alias` field in newly generated local-preview catalog records a recognized, deterministic legacy numeric-version URL after verifying its path, source package length and SHA-256. The existing canonical `pkg` stays unchanged. On subsequent generations, `_kept_versions` carries the validated earlier release entry, and `_keep_checked_numeric_aliases` verifies and explicitly copies its legacy URL even when the incoming version is different (or legacy/unversioned). Missing, corrupted, symlinked or malformed declared aliases refuse prior to creating the new output. Old-to-new and new-to-new same-version checks remain in place. Only selected retained entries contribute their historical aliases.

## Executed GREEN checks
Environment: connected DnBOX cloud, Python 3.13.5, existing GitHub-verified 901/901 dependency closure, isolated local fixture; no Git-network operations.

- Focused test: **1 PASS, 0 FAIL, exit 0** (0.810 s). Raw `n1-pr903-cross-version-GREEN.log`; Git blob `c10e85c34066bda6f4dc5f086352dc39eabe7c46`; SHA-256 `39508ac3c3eea57915c8e789a5a5067a849f2ce246f4d23c2d3e08078dee45da`.
- Integrated selector: `PYTHONDONTWRITEBYTECODE=1 python3 tests/host/test_release_publish.py` **8 PASS, 0 FAIL, exit 0** (2.025 s). Raw `n1-pr903-cross-version-integrated-GREEN.log`; Git blob `01da013099548937ef848fa0adde2cff989b7a27`; SHA-256 `0993a04460e83c02968bb366e843665678fc1dc373f5844fbfce6f1b4f6be821`.
- Test sequence: old generator `0.1.3` (2 files) -> new generator `0.1.3` (4) -> repeat `0.1.3` (4) -> new `0.1.4` (6). Both original legacy `0.1.3` files survived byte-identically beside both `0.1.3` and both `0.1.4` DnBOX canonical files. Catalog still advertises only canonical DnBOX names.
- Both missing and corrupted legacy files were refused before output generation in the same-version and **different-version** paths. The test asserts the output directory was never created.
- Exact executed production/test Git blobs were independently matched to GitHub-created blobs before branch update.

## Boundaries
Only text source, host regression and evidence are changed. This is synthetic local-preview generation, not a release. The existing published 0.0.26 catalog, legacy `_900` product identities and historical public URLs are not altered or independently requalified. No native firmware rebuild, device read/write, HTTP file readback, installer browser test, website deployment, GitHub Actions, merge or publication. Independent exact-head review remains required.
