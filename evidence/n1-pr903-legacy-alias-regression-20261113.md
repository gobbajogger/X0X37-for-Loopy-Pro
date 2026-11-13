# N1 #903: numeric-release legacy filename compatibility, independent P2 repair

## Scope and source

Independent finding: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/903#issuecomment-6862084236

Source parent: `f637e5b139320d28c28cc92c4f55fd5a9fe3544a`, tree `4cb5c7e1c519222a63cac8fc7ba45113c00e5746`. The only production-source change is `tools/publish_nightly.py`; the additional test is `tests/host/test_release_publish.py`. Both remain in the existing `lane/n1-dnbox-package-names-20261113` / draft PR #903.

Before the repair, `_kept_versions` dropped a prior entry with the same numeric release. `make_pages.write_site` copied only files referenced by the new catalog or supplied in `copy_pkgs`, so a prior `x0x37-0.1.3-elite.fwsc` and `x0x37-0.1.3.fwsc` disappeared when the new `DnBOX-0.1.3-SMK37-{model}-{sha8}-hwtest.fwsc` outputs replaced them. The finding was **reproduced**, not inferred from current published 0.0.26.

## Fix

`_keep_checked_numeric_aliases` runs during catalog regeneration, before any output is written. For a matching numeric release, it validates the prior record against the new identity (version/source/tree, exact Elite+Pro product and controller/package SHA-256), requires the previous URL to use either the known old-generator name or current DnBOX name, and verifies the prior file is a regular file with matching size and SHA-256. Old-generator URLs are copied explicitly as compatibility aliases alongside the new canonical names.

A later new-to-new regeneration also retains validated aliases even though the new catalog does not reference them. Unknown prior filenames, missing/corrupt alias bytes, mismatching metadata, or untracked alias files fail closed. This does **not** invent or relabel any existing firmware product identity, nor allow a different version to reuse a numeric release.

## Actual focused regression

The added `test_old_generator_to_new_numeric_republish_keeps_checked_urls` executes the real publish/validation/site-writing functions twice, with the first call applying precisely the old producer's `_new_version` filename expression through a scoped mock. It is an actual synthetic package/site regeneration fixture, **not** a claimed archived binary or a native firmware build. It checks:

1. Old-generator Elite/Pro catalog links and files match the admitted packages.
2. Old-to-new regenerate produces both canonical DnBOX names while keeping both old URLs and bytes.
3. A second new-to-new regenerate keeps all four files.
4. Corrupted or missing historical aliases refuse regeneration before writing an output site.

Environment: authorized DnBOX cloud Linux, Python 3.13.5, isolated scratch source based on the prior **901/901** GitHub-verified dependency closure. No git-network operations. Test source and publisher were edited locally for the red/green run and their exact resulting Git blobs were independently checked before connector commit.

- **RED, old publisher blob `14eb4f0db86b5cb74cc92247d61e4a2b34fac05a`:** `PYTHONDONTWRITEBYTECODE=1 python3 -m unittest tests/host/test_release_publish.py -k old_generator_to_new_numeric_republish_keeps_checked_urls` → 1 test ERROR, expected `FileNotFoundError` for absent `new-generator/firmware/x0x37-0.1.3-elite.fwsc`.
- **GREEN, patched publisher blob `0ef1efd84b8bea872f9afd91b03876777d44dbc9`:** same command → 1 test PASS / exit 0.
- **Integrated GREEN, patched test blob `11f125ca057a920ab76dd215ae50e8d602286ab6`:** `PYTHONDONTWRITEBYTECODE=1 python3 tests/host/test_release_publish.py` → **8 tests PASS, zero failures, exit 0** in 1.458 s.

Raw captured UTF-8 logs are in this same directory:
- `n1-pr903-old-to-new-RED.log`, Git blob `17c917571806ecb43b47eac35874177e10a9ec1c`, SHA-256 `e769034d01a640c68bc3dec6751099aa333162197ccf99acfdb3e9d56b3fdfca`
- `n1-pr903-old-to-new-GREEN.log`, Git blob `3a8d08939a9696d8d29504e21f6f8e371889d311`, SHA-256 `61bb71876967cb6dbfce0a6fb6af310650e246e2979f07d91548054268eb537c`
- `n1-pr903-integrated-alias-GREEN.log`, Git blob `75315bcf8cdd21d6cf8372ca9983a03e1db273ab`, SHA-256 `e5d90651d01cc3c0e1bc653b9282e4fce5c13f34379a7d5ed467dc96a3bde373`

## Remaining limits

The live 0.0.26 nightly catalog still advertises `SMK-37 Elite_900` / `SMK-37 Pro_900` product identities. Those historic identities have no encoded semantic version; a correct DnBOX download filename is **not** internal numeric version proof. Older immutable nightly URLs remain unchanged. HTTP `Content-Disposition`, actual served binary readback, browser Save As and owner physical USB/BLE testing were **NOT_RUN**. No firmware build, test artifact publication, install, release/Pages deployment, protected merge, .github edit or device write was performed. Independent re-review is required.
