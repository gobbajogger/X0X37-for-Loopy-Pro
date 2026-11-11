# Installer dual digests (FP-001)

Stage-7 qualification and live-page checks use **two** digests. They are not interchangeable.

| Name | Meaning | Typical source |
| --- | --- | --- |
| `installer_source_fingerprint` | SHA-256 of the nightly source composite (`installerFingerprint()` over installer HTML/JS inputs and render scripts) | `qualification.installer_sha256` |
| `served_install_html_sha256` | SHA-256 of the bytes actually served at `install/index.html` | Fresh fetch of the live or prepared installer page |

Nightly `prepare:candidate` / `validateCatalog` keep requiring `qualification.installer_sha256 === installerFingerprint()`. Do **not** put the served HTML digest into `installer_sha256`; that would break or weaken the source gate.

Firmware stage evidence (CHECKS, HANDOFF, `digests.json`) must name both fields. A single “Installer FP” line that pretends one value covers both is a FAIL.

Local host check:

```sh
python3 tools/check_installer_digests.py evidence/<slug>-stageN/digests.json --fetch
```

The check fails when evidence conflates the two digests, when `installer_sha256` stands alone without the dual fields, when the two named digests are equal, or when `served_install_html_sha256` does not match fetched install HTML.
