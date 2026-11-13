# N1 / PR #903: integrated packaging-name regression

**Outcome: PASS (host integration fixture only).** The firmware release, installer site and live device remain unchanged.

## Exact checked inputs

- Candidate source commit: `c0abf925c76aa62e3691def9385dd4613be55b53`
- Candidate Git root tree: `4c9abc9946e79c140f09abb4566b24401c613391`
- Source test: `tests/host/test_release_publish.py`, Git blob `dcc8a8bbe839f5bd53df98a815c0fecff6ae8fe9`
- Packaging producer: `tools/publish_nightly.py`, Git blob `14eb4f0db86b5cb74cc92247d61e4a2b34fac05a`
- Main-linked dependency scopes read using the connected GitHub Git Tree / file APIs at that exact candidate tree:
  - `tools/` tree `964884e6922660e1204ac78784f7abbcabd48f9c`: 86 direct file blobs
  - `web/` tree `477a00b801d43fbc89a271418d05316e1a2790c1`: 55 direct file blobs
  - `tests/host/` tree `6e79a5e6850fb35d44ee7f374fe5c721837fb904`: 703 direct file blobs
  - `docs/` tree `2f9a49b100a3ca97a37e2dcb0730d2e95f875579`: 57 direct file blobs
- Each of those **901/901** local source file blobs matched the GitHub tree's recorded SHA-1 after exact-ref copies were delivered into an isolated scratch directory. This checks those specified scopes, **not** every file of the 5,000+-file full repository tree. A preexisting cloud snapshot was never assumed to be exact.
- All file transfer for missing/changed source content used the connected GitHub connector, without git-network clone/fetch/push. Local `git init` inside synthetic temporary fixtures was part of the existing test.

## Executed result

Host: authorized DnBOX cloud Linux; Python `3.13.5`. Working directory: isolated scratch snapshot with the pinned candidate files and matching dependency blobs.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tests/host/test_release_publish.py
```

```text
.......
----------------------------------------------------------------------
Ran 7 tests in 1.082s

OK
N1_FINAL_EXIT=0
```

Full captured output: [n1-pr903-integrated-run-20261113.log](n1-pr903-integrated-run-20261113.log).
- Raw output size: **1,274 bytes**
- SHA-256: `11ebae3a2332ac240dcf27cdf22dba9bdc7f37e786af56e295909eb5abb4a8f5`
- Git blob SHA-1: `d085ae193316a3116a49c37c3da9d7e1f670f432`, verified by remote host and GitHub readback.
- 7 top-level `unittest` methods passed, with additional negative subtests. The isolated 22-case helper exercise from the previous checkpoint was **not** separately repeated. This run uses non-executable synthetic package fixtures; it is not a native build, live browser, actual package download or hardware check.

## Remaining package-identity gaps

1. Live NIGHTLY catalog `gh-pages@5734c215c233fb1431d80e26558f7ac662c6a992`, catalog blob `24a8b208aa01d539b5ef4e23af73d9c4aa2ac3d8`, labels latest as **0.0.26** and correctly names its Elite/Pro public URLs `DnBOX-0.0.26-SMK37-Elite-94197fc8-hwtest.fwsc` and `DnBOX-0.0.26-SMK37-Pro-94197fc8-hwtest.fwsc`. But catalog product identities remain **`SMK-37 Elite_900` / `SMK-37 Pro_900`**, the historically unversioned product codec. This fixture does not prove that the public 0.0.26 binary embeds a numeric version in the product identity. Renaming a file cannot alter package bytes.
2. The new numeric-identity naming producer applies to **future packages whose embedded product actually carries numeric X.Y.Z**. It intentionally preserves legacy `_900` fallback names for local previews; it does not retrofit current installed 0.0.26 or assign a new number. Unnumbered public publication still needs policy/qualification from its separate owner, not a guessed version.
3. The prior **72** public model links in 36 older catalog entries remain under `x0x37-*.fwsc` immutable URLs. Keep those paths accessible. Any separately presented user-facing alias must retain the original checksum/source/model truth; it is outside this source-only PR.
4. HTTP server `Content-Disposition`, actual served firmware bytes, current browser Save As outcome and physical USB/MIDI/BLE model names were **NOT_RUN**. Existing installer code derives `download` from the selected catalog basename after creating a verified Blob; the website owner owns live browser/HTTP acceptance. Firmware identity handoff is recorded on #869.

## Boundaries

No .fwsc, controller, SDK, release, catalog, browser HTML, firmware/DSP/USB/BLE source or binary output was committed or published. No target builds, broad suite, device writes, .github changes, protected merge, force push or git-network operations were performed. This receipt establishes the focused source regression only; independent review is still required before any merge or publisher adoption.
